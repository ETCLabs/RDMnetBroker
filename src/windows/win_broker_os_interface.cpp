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
#include <cassert>
#include <Windows.h>
#include <ShlObj.h>
#include <datetimeapi.h>
#include "service_utils.h"

constexpr const WCHAR kRelativeConfFileName[] = L"\\ETC\\RDMnetBroker\\broker.conf";
static const std::vector<std::wstring> kRelativeLogFilePath = {L"ETC", L"RDMnetBroker"};
static const std::wstring kLogFileName = L"broker.log";
static constexpr int kMaxLogRotationFiles = 5;

std::string ConvertWstringToUtf8(const std::wstring& str)
{
  // Convert the path to UTF-8
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, NULL, 0, NULL, NULL);
  if (size_needed > 0)
  {
    auto buf = std::make_unique<char[]>(size_needed);
    int convert_res = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, buf.get(), size_needed, NULL, NULL);
    if (convert_res > 0)
    {
      return buf.get();
    }
  }
  return std::string{};
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

  return ConvertWstringToUtf8(log_file_path_);
}

bool WindowsBrokerOsInterface::OpenLogFile()
{
  assert(!log_file_);

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

  DWORD rotate_result = RotateLogs();

  log_file_ = _wfsopen(log_file_path_.c_str(), L"w", _SH_DENYWR);
  if (!log_file_)
  {
    std::cout << "FATAL: Error opening log file for writing: " << errno << '\n';
    return false;
  }

  // Write an initial message to the log file
  EtcPalLogTimeParams time;
  GetLogTime(time);
  char initial_msg[100];
  _snprintf_s<100>(initial_msg, _TRUNCATE, "Starting RDMnet Broker Service on %04d-%02d-%02d at %02d:%02d:%02d...\n",
                   time.year, time.month, time.day, time.hour, time.minute, time.second);
  fwrite(initial_msg, sizeof(char), strnlen_s(initial_msg, 100), log_file_);

  // Write an error message to the log file if it is open and there was an error rotating the logs
  if (rotate_result != 0)
  {
    wchar_t error_msg[512];
    GetLastErrorMessage(rotate_result, error_msg, 512);
    auto log_msg = "WARNING: rotating log files failed with error: \"" + ConvertWstringToUtf8(error_msg) + "\"\n";
    fwrite(log_msg.c_str(), sizeof(char), log_msg.size(), log_file_);
  }

  fflush(log_file_);
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
  std::ifstream conf_file(conf_file_path);

  return std::make_pair(ConvertWstringToUtf8(conf_file_path), std::move(conf_file));
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

DWORD WindowsBrokerOsInterface::RotateLogs()
{
  // If we don't have the primary log file, just stop early
  DWORD file_attr = GetFileAttributes(log_file_path_.c_str());
  if (file_attr == INVALID_FILE_ATTRIBUTES)
    return 0;

  int rotate_number = 1;
  auto LogBackupFileName = [&](int rotate_number) { return log_file_path_ + L"." + std::to_wstring(rotate_number); };

  // Determine the highest log backup file that already exists on the system
  for (; rotate_number < kMaxLogRotationFiles; ++rotate_number)
  {
    file_attr = GetFileAttributes(LogBackupFileName(rotate_number).c_str());
    if (file_attr == INVALID_FILE_ATTRIBUTES)
      break;
  }

  // Copy each file to the next higher-numbered file, starting with the highest and working down.
  while (rotate_number-- > 0)
  {
    std::wstring src_file_name;
    if (rotate_number == 0)
      src_file_name = log_file_path_;
    else
      src_file_name = LogBackupFileName(rotate_number);

    if (!CopyFile(src_file_name.c_str(), LogBackupFileName(rotate_number + 1).c_str(), FALSE))
    {
      return GetLastError();
    }
  }
  return 0;
}
