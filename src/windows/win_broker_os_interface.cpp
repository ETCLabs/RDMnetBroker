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

#include "win_broker_os_interface.h"

#include <iostream>
#include <memory>
#include <Windows.h>
#include <ShlObj.h>

constexpr const WCHAR kRelativeConfFileName[] = L"\\ETC\\RDMnetBroker\\broker.conf";
static const std::vector<std::wstring> kRelativeLogFilePath = {L"ETC", L"RDMnetBroker"};
static const std::wstring kLogFileName = L"broker.log";

std::unique_ptr<char[]> ConvertPathToUtf8(const std::wstring& conf_file_path)
{
  // Convert the path to UTF-8
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, conf_file_path.c_str(), -1, NULL, 0, NULL, NULL);
  if (size_needed > 0)
  {
    auto buf = std::make_unique<char[]>(size_needed);
    int convert_res = WideCharToMultiByte(CP_UTF8, 0, conf_file_path.c_str(), -1, buf.get(), size_needed, NULL, NULL);
    if (convert_res > 0)
    {
      return buf;
    }
  }
  return std::unique_ptr<char[]>();
}

WindowsBrokerOsInterface::WindowsBrokerOsInterface()
{
  PWSTR program_data_path;
  HRESULT get_known_folder_res = SHGetKnownFolderPath(FOLDERID_ProgramData, 0, NULL, &program_data_path);
  if (get_known_folder_res == S_OK)
  {
    program_data_path_ = program_data_path;
    CoTaskMemFree(program_data_path);

    // Build the log file path on top of it
    log_file_path_ = program_data_path_;
    for (const auto& intermediate_dir : kRelativeLogFilePath)
    {
      log_file_path_ += L"\\" + intermediate_dir;
    }
    log_file_path_ += L"\\" + kLogFileName;
  }
}

WindowsBrokerOsInterface::~WindowsBrokerOsInterface()
{
  if (log_file_)
    fclose(log_file_);
}

std::string WindowsBrokerOsInterface::GetLogFilePath() const
{
  if (log_file_path_.empty())
    return std::string{};

  auto convert_res = ConvertPathToUtf8(log_file_path_);
  if (convert_res)
    return convert_res.get();
  else
    return std::string{};
}

bool WindowsBrokerOsInterface::OpenLogFile()
{
  if (program_data_path_.empty())
  {
    std::cout << "FATAL: Could not get location of ProgramData directory.\n";
    return false;
  }

  std::wstring intermediate_dir_path = program_data_path_;
  for (const auto& intermediate_dir : kRelativeLogFilePath)
  {
    intermediate_dir_path += L"\\" + intermediate_dir;
    if (!CreateDirectoryW(intermediate_dir_path.c_str(), NULL))
    {
      DWORD error = GetLastError();
      if (error != ERROR_ALREADY_EXISTS)
      {
        // Something went wrong creating an intermediate directory.
        std::wcout << L"FATAL: Error creating intermediate directory \"" << intermediate_dir << L"\" in ProgramData: "
                   << error << L"\n";
        return false;
      }
    }
  }

  RotateLogs();
  log_file_ = _wfsopen(log_file_path_.c_str(), L"w", _SH_DENYWR);
  if (!log_file_)
  {
    std::cout << "FATAL: Error opening log file for writing: " << errno << '\n';
    return false;
  }
  return true;
}

std::pair<std::string, std::ifstream> WindowsBrokerOsInterface::GetConfFile(rdmnet::BrokerLog& log)
{
  if (program_data_path_.empty())
  {
    log.Critical("FATAL: Could not get location of ProgramData directory.\n");
    return std::make_pair(std::string{}, std::ifstream{});
  }

  std::wstring conf_file_path = program_data_path_ + kRelativeConfFileName;
  auto conf_file_path_utf8 = ConvertPathToUtf8(conf_file_path);

  std::ifstream conf_file(conf_file_path);
  return std::make_pair(std::string{conf_file_path_utf8.get()}, std::move(conf_file));
}

void WindowsBrokerOsInterface::GetLogTime(EtcPalLogTimeParams& time)
{
  int utc_offset = 0;

  TIME_ZONE_INFORMATION tzinfo;
  switch (GetTimeZoneInformation(&tzinfo))
  {
    case TIME_ZONE_ID_UNKNOWN:
    case TIME_ZONE_ID_STANDARD:
      utc_offset = -(tzinfo.Bias + tzinfo.StandardBias);
      break;
    case TIME_ZONE_ID_DAYLIGHT:
      utc_offset = -(tzinfo.Bias + tzinfo.DaylightBias);
      break;
    default:
      break;
  }

  SYSTEMTIME win_time;
  GetLocalTime(&win_time);
  time.year = win_time.wYear;
  time.month = win_time.wMonth;
  time.day = win_time.wDay;
  time.hour = win_time.wHour;
  time.minute = win_time.wMinute;
  time.second = win_time.wSecond;
  time.msec = win_time.wMilliseconds;
  time.utc_offset = utc_offset;
}

void WindowsBrokerOsInterface::OutputLogMsg(const std::string& str)
{
  if (log_file_)
  {
    fwrite(str.c_str(), sizeof(char), str.length(), log_file_);
    fwrite("\n", sizeof(char), 1, log_file_);
    fflush(log_file_);
  }
}

void WindowsBrokerOsInterface::RotateLogs()
{
  // TODO
}
