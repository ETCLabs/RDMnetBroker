/******************************************************************************
 * Copyright 2019 ETC Inc.
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

#include "win_broker_shell.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <Windows.h>
#include <ShlObj.h>

constexpr const WCHAR kRelativeConfFileName[] = L"\\ETC\\RDMnetBroker\\broker.conf";

bool WindowsBrokerShell::LoadBrokerConfig(rdmnet::BrokerLog& log)
{
  PWSTR program_data_path;
  HRESULT get_known_folder_res = SHGetKnownFolderPath(FOLDERID_ProgramData, 0, NULL, &program_data_path);
  if (get_known_folder_res != S_OK)
  {
    log.Critical("FATAL: Error getting location of ProgramData directory: %ld\n", get_known_folder_res);
    return false;
  }

  std::wstring conf_file_path = std::wstring(program_data_path) + kRelativeConfFileName;
  CoTaskMemFree(program_data_path);

  std::ifstream conf_file(conf_file_path);
  if (conf_file.is_open())
  {
    auto parse_res = broker_config_.Read(conf_file, &log);
    if (parse_res != BrokerConfig::ParseResult::kOk)
    {
      log.Critical("FATAL: Error while reading configuration file.");
      return false;
    }
  }
  else
  {
    // Convert the path to UTF-8
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, conf_file_path.c_str(), -1, NULL, 0, NULL, NULL);
    if (size_needed > 0)
    {
      auto buf = std::make_unique<char[]>(size_needed);
      int convert_res = WideCharToMultiByte(CP_UTF8, 0, conf_file_path.c_str(), -1, buf.get(), size_needed, NULL, NULL);
      if (convert_res > 0)
      {
        // Remove the NULL from being included in the string's count
        log.Notice("No configuration file located at path \"%s\". Proceeding with default settings...", buf.get());
      }
      else
      {
        log.Notice("No configuration file found. Proceeding with default settings...");
      }
    }
    broker_config_.SetDefaults();
  }
  return true;
}
