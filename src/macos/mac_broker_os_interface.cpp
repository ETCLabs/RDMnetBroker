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

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

static constexpr const char* kLogDirectoryPath[] = {"usr", "local", "var", "log"};
static constexpr size_t kLogDirectoryDepth = sizeof(kLogFilePath) / sizeof(const char*);
static constexpr const char* kLogFileName = "RDMnetBroker.log";
static constexpr mode_t kLogFileMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

static constexpr const char* kConfigDirectoryPath[] = {"usr", "local", "etc"};
static constexpr size_t kConfigFDirectoryDepth = sizeof(kConfigFilePath) / sizeof(const char*);
static constexpr const char* kConfigFileName = "RDMnetBroker.conf";
static constexpr mode_t kConfigFileMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

std::string ConstructDirectoryPath(const char** dir_elements, size_t num_dir_elements)
{
  std::string result = "/";
  for(size_t i = 0u; i < num_dir_elements; ++i)
  {
    result.append(dir_elements[i]);
    result.append("/");
  }

  return result;
}

std::string ConstructFilePath(const char** dir_elements, size_t num_dir_elements, const char* file_name)
{
  auto result = ConstructDirectoryPath(dir_elements, num_dir_elements);
  result.append(kLogFileName);
  return result;
}

bool CreateFoldersAsNeeded(const char** dir_elements, size_t num_dir_elements, mode_t mode)
{
  std::string mkdir_path;
  for(size_t i = 0u; i < num_dir_elements; ++i)
  {
    mkdir_path.append("/");
    mkdir_path.append(dir_elements[i]);
    if((mkdir(mkdir_path.c_str(), mode) != 0) && (errno != EEXIST))
      return false;
  }

  return true;
}

std::string MacBrokerOsInterface::GetLogFilePath() const
{
  return ConstructFilePath(kLogDirectoryPath, kLogDirectoryDepth, kLogFileName);
}

bool MacBrokerOsInterface::OpenLogFile()
{
  // TODO
}

std::pair<std::string, std::ifstream> MacBrokerOsInterface::GetConfFile(etcpal::Logger& log)
{
  // TODO
}

etcpal::LogTimestamp MacBrokerOsInterface::GetLogTimestamp()
{
  // TODO
}

void MacBrokerOsInterface::HandleLogMessage(const EtcPalLogStrings& strings)
{
  // TODO
}

etcpal::Error RotateLogs()
{
  // TODO
}
