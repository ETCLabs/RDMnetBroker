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

class PathComponent
{
public:
  PathComponent(const std::string& name, mode_t mode) : name_(name), mode_(mode) {}

  const std::string& name() const { return name_; }
  mode_t             mode() const { return mode_; }

private:
  std::string  name_;
  const mode_t mode_;
};

class FilePath
{
public:
  FilePath(const std::vector<PathComponent>& directory, const PathComponent& file) : directory_(directory), file_(file)
  {
  }

  std::string ToString() const;

  const std::vector<PathComponent>& directory() const { return directory_; }
  const PathComponent&              file() const { return file_; }

private:
  std::vector<PathComponent> directory_;
  PathComponent              file_;
};

// Directory mode = rwxr-xr-x to match precedent set by /usr
// Log file mode = rw-r--r-- because it only needs to be written to by the service
// Config file mode = rw-rw-rw- to allow configuration of the service without elevated permissions
static constexpr mode_t kDirectoryMode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
static constexpr mode_t kLogFileMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
static constexpr mode_t kConfigFileMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

// Files are put into an RDMnetBroker folder in each respective location. Not only does this help with organization, but
// it also helps support increased file permissions (i.e. setting kConfigFileMode on parent RDMnetBroker directory).
static const FilePath kLogPath = FilePath(
    {PathComponent("usr", kDirectoryMode), PathComponent("local", kDirectoryMode), PathComponent("var", kDirectoryMode),
     PathComponent("log", kDirectoryMode), PathComponent("RDMnetBroker", kDirectoryMode)},
    PathComponent("broker.log", kLogFileMode));
static const FilePath kConfigPath =
    FilePath({PathComponent("usr", kDirectoryMode), PathComponent("local", kDirectoryMode),
              PathComponent("etc", kDirectoryMode), PathComponent("RDMnetBroker", kDirectoryMode | kConfigFileMode)},
             PathComponent("broker.conf", kConfigFileMode));

static constexpr int kMaxLogRotationFiles = 5;

bool CreateFoldersAsNeeded(const FilePath& path)
{
  std::string mkdir_path("/");
  for (const auto& folder : path.directory())
  {
    mkdir_path.append(folder.name());
    mkdir_path.append("/");  // Make sure there's a trailing / so mkdir_path is considered a directory

    bool folder_exists = (access(mkdir_path.c_str(), F_OK) == 0);
    if (!folder_exists)
    {
      if (mkdir(mkdir_path.c_str(), folder.mode()) == -1)
        return false;

      // We might need to force extra write permissions
      if (chmod(mkdir_path.c_str(), folder.mode()) == -1)
        return false;
    }
  }

  return true;
}

bool CreateFileIfNeeded(const FilePath& path)
{
  // Call open for this to prevent truncation.
  int fd = open(path.ToString().c_str(), O_RDWR | O_CREAT, path.file().mode());

  if (fd == -1)
    return false;

  if (close(fd) == -1)
    return false;

  // We might need to force extra write permissions
  return (chmod(path.ToString().c_str(), path.file().mode()) == 0);
}

bool SetUpFileAtPath(const FilePath& path)
{
  if (!CreateFoldersAsNeeded(path))
  {
    std::cout << "FATAL: Could not create directory for '" << path.ToString() << "' (" << strerror(errno) << ").\n";
    return false;
  }

  if (!CreateFileIfNeeded(path))
  {
    std::cout << "FATAL: Could not create file '" << path.ToString() << "' (" << strerror(errno) << ").\n";
    return false;
  }

  return true;
}

bool RotateLog(const FilePath& src_path, const FilePath& dest_path)
{
  bool success = false;
  int  src_fd = open(src_path.ToString().c_str(), O_RDONLY);
  if (src_fd >= 0)
  {
    int dest_fd = creat(dest_path.ToString().c_str(), dest_path.file().mode());
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
      return kLogPath;

    return FilePath(kLogPath.directory(), PathComponent(kLogPath.file().name() + "." + std::to_string(rotate_number),
                                                        kLogPath.file().mode()));
  };

  // Determine the highest log backup file that already exists on the system
  int rotate_number = 1;
  for (; rotate_number < kMaxLogRotationFiles; ++rotate_number)
  {
    auto backup_path = LogBackupFilePath(rotate_number);
    bool file_exists = (access(backup_path.ToString().c_str(), F_OK) == 0);
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

void InitializeNewConfig()
{
  // Assuming the file has already been successfully created with permissions by this point.
  std::ofstream conf_file(kConfigPath.ToString());
  conf_file << "{\n}\n";  // Initialize with the empty JSON object {}
  conf_file.close();
}

std::string FilePath::ToString() const
{
  std::string result = "/";
  for (const auto& folder : directory_)
  {
    result.append(folder.name());
    result.append("/");
  }

  result.append(file_.name());
  return result;
}

MacBrokerOsInterface::~MacBrokerOsInterface()
{
  if (log_stream_.is_open())
    log_stream_.close();
}

std::string MacBrokerOsInterface::GetLogFilePath() const
{
  return kLogPath.ToString();
}

bool MacBrokerOsInterface::OpenLogFile()
{
  if (!SetUpFileAtPath(kLogPath))
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
  bool new_config = (access(kConfigPath.ToString().c_str(), F_OK) != 0);

  if (!SetUpFileAtPath(kConfigPath))
  {
    log.Critical("FATAL: creating config file failed with error: \"%s\"\n", strerror(errno));
    return std::make_pair(std::string{}, std::ifstream{});
  }

  if (new_config)
    InitializeNewConfig();

  std::string   conf_file_path = kConfigPath.ToString();
  std::ifstream conf_file(conf_file_path);

  return std::make_pair(conf_file_path, std::move(conf_file));
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
