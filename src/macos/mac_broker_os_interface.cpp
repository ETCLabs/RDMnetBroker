/******************************************************************************
 * Copyright 2022 ETC Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ******************************************************************************
 * This file is a part of RDMnetBroker. For more information, go to:
 * https://github.com/ETCLabs/RDMnetBroker
 *****************************************************************************/

#include "mac_broker_os_interface.h"

#include "broker_version.h"

#include <cmath>
#include <copyfile.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include <CoreFoundation/CoreFoundation.h>

// Log file mode = rw-r--r-- because it only needs to be written to by the service
static constexpr mode_t kLogFileMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

static constexpr char* kLogFilePath = "/usr/local/var/log/RDMnetBroker/broker.log";
static constexpr char* kConfigFilePath = "/usr/local/etc/RDMnetBroker/broker.conf";

static constexpr int kMaxLogRotationFiles = 5;

bool SetUpLogFile()
{
  // The installer has already set up the log directory. We only need to create the file here.
  bool success = false;

  // Call open for this to prevent truncation of an existing log.
  int fd = open(kLogFilePath, O_RDWR | O_CREAT, kLogFileMode);
  if (fd >= 0)
    success = (close(fd) == 0);

  if (!success)
    std::cout << "FATAL: Could not create file '" << kLogFilePath << "' (" << strerror(errno) << ").\n";

  return success;
}

bool RotateLog(const std::string& src_path, const std::string& dest_path)
{
  bool success = false;

  int src_fd = open(src_path.c_str(), O_RDONLY);
  if (src_fd >= 0)
  {
    int dest_fd = creat(dest_path.c_str(), kLogFileMode);
    if (dest_fd >= 0)
    {
      if (fcopyfile(src_fd, dest_fd, nullptr, COPYFILE_ALL) == 0)
        success = true;

      close(dest_fd);
    }

    close(src_fd);
  }

  return success;
}

bool RotateLogs()
{
  auto LogBackupFilePath = [&](int rotate_number) {
    if (rotate_number == 0)
      return std::string(kLogFilePath);

    return (std::string(kLogFilePath) + "." + std::to_string(rotate_number));
  };

  // Determine the highest log backup file that already exists on the system
  int rotate_number = 1;
  for (; rotate_number < kMaxLogRotationFiles; ++rotate_number)
  {
    auto backup_path = LogBackupFilePath(rotate_number);
    bool file_exists = (access(backup_path.c_str(), F_OK) == 0);
    if (!file_exists)
      break;
  }

  // Copy each file to the next higher-numbered file, starting with the highest and working down.
  while (--rotate_number >= 0)
  {
    if (!RotateLog(LogBackupFilePath(rotate_number), LogBackupFilePath(rotate_number + 1)))
      return false;
  }

  return true;
}

MacBrokerOsInterface::~MacBrokerOsInterface()
{
  if (log_stream_.is_open())
    log_stream_.close();
}

std::string MacBrokerOsInterface::GetLogFilePath() const
{
  return kLogFilePath;
}

bool MacBrokerOsInterface::OpenLogFile()
{
  if (!SetUpLogFile())
    return false;

  bool rotate_error = !RotateLogs();

  log_stream_.open(GetLogFilePath());

  // Write an initial message to the log file
  auto time = GetLogTimestamp();
  log_stream_ << "Starting RDMnet Broker Service version " << BrokerVersion::VersionString() << " on "
              << std::setfill('0') << std::setw(4) << time.get().year << "-" << std::setw(2) << time.get().month << "-"
              << std::setw(2) << time.get().day << " at " << std::setw(2) << time.get().hour << ":" << std::setw(2)
              << time.get().minute << ":" << std::setw(2) << time.get().second << "...\n";

  // Write an error message to the log file if it is open and there was an error rotating the logs
  if (rotate_error)
    log_stream_ << "WARNING: rotating log files failed with error: \"" << strerror(errno) << "\"\n";

  log_stream_.flush();

  return true;
}

std::pair<std::string, std::ifstream> MacBrokerOsInterface::GetConfFile(etcpal::Logger& log)
{
  // The installer has already set up the config directory and file.
  std::ifstream conf_file(kConfigFilePath);
  return std::make_pair(kConfigFilePath, std::move(conf_file));
}

etcpal::LogTimestamp MacBrokerOsInterface::GetLogTimestamp()
{
  CFTimeZoneRef   time_zone = CFTimeZoneCopySystem();
  CFAbsoluteTime  abs_time = CFAbsoluteTimeGetCurrent();
  CFGregorianDate date = CFAbsoluteTimeGetGregorianDate(abs_time, time_zone);
  unsigned int    msec = static_cast<unsigned int>(fmod(date.second, 1.0) * 1000.0);
  CFTimeInterval  utc_offset = CFTimeZoneGetSecondsFromGMT(time_zone, abs_time) / 60.0;

  return etcpal::LogTimestamp(static_cast<unsigned int>(date.year), static_cast<unsigned int>(date.month),
                              static_cast<unsigned int>(date.day), static_cast<unsigned int>(date.hour),
                              static_cast<unsigned int>(date.minute), static_cast<unsigned int>(date.second), msec,
                              static_cast<int>(utc_offset));
}

void MacBrokerOsInterface::HandleLogMessage(const EtcPalLogStrings& strings)
{
  if (log_stream_.is_open())
  {
    log_stream_ << strings.human_readable << "\n";
    log_stream_.flush();
  }
}
