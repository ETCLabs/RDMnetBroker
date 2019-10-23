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

#include "win_broker_log.h"

#include <iostream>
#include <string>
#include <ShlObj.h>
#include <share.h>

static const std::vector<std::wstring> kRelativeLogFilePath = {L"ETC", L"RDMnetBroker"};
constexpr const WCHAR kLogFileName[] = L"broker.log";

bool WindowsBrokerLog::OnStartup()
{
  std::wstring log_file_path = GetOrCreateLogFilePath();
  if (log_file_path.empty())
    return false;

  RotateLogs(log_file_path);
  log_file_ = _wfsopen(log_file_path.c_str(), L"w", _SH_DENYWR);
  if (!log_file_)
  {
    std::cout << "FATAL: Error opening log file for writing: " << errno << '\n';
    return false;
  }
  return true;
}

std::wstring WindowsBrokerLog::GetOrCreateLogFilePath()
{
  PWSTR program_data_path;
  HRESULT get_known_folder_res = SHGetKnownFolderPath(FOLDERID_ProgramData, 0, NULL, &program_data_path);
  if (get_known_folder_res != S_OK)
  {
    std::cout << "FATAL: Error getting location of ProgramData directory: " << get_known_folder_res << '\n';
    return std::wstring{};
  }

  std::wstring log_file_path = program_data_path;
  CoTaskMemFree(program_data_path);

  for (const auto& intermediate_dir : kRelativeLogFilePath)
  {
    log_file_path += L"\\" + intermediate_dir;
    if (!CreateDirectoryW(log_file_path.c_str(), NULL))
    {
      DWORD error = GetLastError();
      if (error != ERROR_ALREADY_EXISTS)
      {
        // Something went wrong creating an intermediate directory.
        std::wcout << L"FATAL: Error creating intermediate directory \"" << intermediate_dir << L"\" in ProgramData: "
                   << error << L"\n";
        return std::wstring{};
      }
    }
  }
  log_file_path += L"\\";
  log_file_path += kLogFileName;

  return log_file_path;
}

void WindowsBrokerLog::OnShutdown()
{
  if (log_file_)
    fclose(log_file_);
}

void WindowsBrokerLog::GetTimeFromCallback(EtcPalLogTimeParams& time)
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

void WindowsBrokerLog::OutputLogMsg(const std::string& str)
{
  if (log_file_)
  {
    fwrite(str.c_str(), sizeof(char), str.length(), log_file_);
    fwrite("\n", sizeof(char), 1, log_file_);
    fflush(log_file_);
  }
}

void WindowsBrokerLog::RotateLogs(const std::wstring& log_file_path)
{
}
