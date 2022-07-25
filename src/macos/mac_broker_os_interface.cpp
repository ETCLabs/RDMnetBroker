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

#include <array>
#include <copyfile.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <CoreFoundation/CoreFoundation.h>

static constexpr std::array<const char*, 5u> kLogDirectoryPath = {"usr", "local", "var", "log", "RDMnetBroker"};
static constexpr const char*                 kLogFileName = "broker.log";

static constexpr std::array<const char*, 4u> kConfigDirectoryPath = {"usr", "local", "etc", "RDMnetBroker"};
static constexpr const char*                 kConfigFileName = "broker.conf";

static constexpr mode_t kDirectoryMode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
static constexpr mode_t kFileMode = S_IRWXU | S_IRWXG | S_IRWXO;

static constexpr int kMaxLogRotationFiles = 5;

template <size_t num_dir_elements>
std::string ConstructDirectoryPath(const std::array<const char*, num_dir_elements>& dir_elements)
{
  std::string result = "/";
  for (size_t i = 0u; i < num_dir_elements; ++i)
  {
    result.append(dir_elements[i]);
    result.append("/");
  }

  return result;
}

template <size_t num_dir_elements>
std::string ConstructFilePath(const std::array<const char*, num_dir_elements>& dir_elements, const char* file_name)
{
  auto result = ConstructDirectoryPath(dir_elements);
  result.append(kLogFileName);
  return result;
}

template <size_t num_dir_elements>
bool CreateFoldersAsNeeded(const std::array<const char*, num_dir_elements>& dir_elements)
{
  std::string mkdir_path;
  for (size_t i = 0u; i < num_dir_elements; ++i)
  {
    mkdir_path.append("/");
    mkdir_path.append(dir_elements[i]);
    if ((mkdir(mkdir_path.c_str(), kDirectoryMode) != 0) && (errno != EEXIST))
      return false;
  }

  return true;
}

bool CreateFileIfNeeded(const std::string& file_path)
{
  // Call open for this so that the file mode can be set up.
  int fd = open(file_path.c_str(), O_RDWR | O_CREAT, kFileMode);

  if (fd == -1)
    return false;

  return (close(fd) != -1);
}

template <size_t num_dir_elements>
bool SetUpFileAtPath(const std::array<const char*, num_dir_elements>& dir_elements, const char* file_name)
{
  std::string path = ConstructFilePath(dir_elements, file_name);
  if (!CreateFoldersAsNeeded(dir_elements))
  {
    std::cout << "FATAL: Could not create directory for '" << path << "' (" << strerror(errno) << ").\n";
    return false;
  }

  if (!CreateFileIfNeeded(path))
  {
    std::cout << "FATAL: Could not create file '" << path << "' (" << strerror(errno) << ").\n";
    return false;
  }

  return true;
}

template <size_t num_dir_elements>
bool FileExists(const std::array<const char*, num_dir_elements>& dir_elements, const char* file_name)
{
  std::string path = ConstructFilePath(dir_elements, file_name);
  return (access(path.c_str(), F_OK) == 0);
}

template <size_t num_dir_elements>
bool CopyFileInDirectory(const std::array<const char*, num_dir_elements>& dir_elements,
                         const char*                                      from_name,
                         const char*                                      to_name)
{
  std::string from_path = ConstructFilePath(dir_elements, from_name);
  std::string to_path = ConstructFilePath(dir_elements, to_name);

  bool success = false;
  int  from_fd = open(from_path.c_str(), O_RDONLY);
  if (from_fd >= 0)
  {
    int to_fd = creat(to_path.c_str(), kFileMode);
    if (to_fd >= 0)
    {
      if (fcopyfile(from_fd, to_fd, nullptr, COPYFILE_ALL) == 0)
        success = true;

      close(to_fd);
    }
    close(from_fd);
  }

  return success;
}

bool RotateLogs()
{
  auto LogBackupFileName = [&](int rotate_number) {
    return std::string(kLogFileName) + "." + std::to_string(rotate_number);
  };

  // Determine the highest log backup file that already exists on the system
  int rotate_number = 1;
  for (; rotate_number < kMaxLogRotationFiles; ++rotate_number)
  {
    if (!FileExists(kLogDirectoryPath, LogBackupFileName(rotate_number).c_str()))
      break;
  }

  // Copy each file to the next higher-numbered file, starting with the highest and working down.
  while (--rotate_number >= 0)
  {
    std::string src_file_name;
    if (rotate_number == 0)
      src_file_name = kLogFileName;
    else
      src_file_name = LogBackupFileName(rotate_number);

    if (!CopyFileInDirectory(kLogDirectoryPath, src_file_name.c_str(), LogBackupFileName(rotate_number + 1).c_str()))
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
  return ConstructFilePath(kLogDirectoryPath, kLogFileName);
}

bool MacBrokerOsInterface::OpenLogFile()
{
  if (!SetUpFileAtPath(kLogDirectoryPath, kLogFileName))
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

  return true;
}

std::pair<std::string, std::ifstream> MacBrokerOsInterface::GetConfFile(etcpal::Logger& log)
{
  if (!SetUpFileAtPath(kConfigDirectoryPath, kConfigFileName))
  {
    log.Critical("FATAL: creating config file failed with error: \"%s\"\n", strerror(errno));
    return std::make_pair(std::string{}, std::ifstream{});
  }

  std::string   conf_file_path = ConstructFilePath(kConfigDirectoryPath, kConfigFileName);
  std::ifstream conf_file(conf_file_path);

  return std::make_pair(conf_file_path, std::move(conf_file));
}

etcpal::LogTimestamp MacBrokerOsInterface::GetLogTimestamp()
{
  CFTimeZoneRef   time_zone = CFTimeZoneCopySystem();
  CFAbsoluteTime  abs_time = CFAbsoluteTimeGetCurrent();
  CFGregorianDate date = CFAbsoluteTimeGetGregorianDate(abs_time, time_zone);
  CFTimeInterval  utc_offset = CFTimeZoneGetSecondsFromGMT(time_zone, abs_time) / 60.0;

  return etcpal::LogTimestamp(date.year, date.month, date.day, date.hour, date.minute, date.second, 0u, utc_offset);
}

void MacBrokerOsInterface::HandleLogMessage(const EtcPalLogStrings& strings)
{
  if (log_stream_.is_open())
    log_stream_ << strings.human_readable << "\n";
}
