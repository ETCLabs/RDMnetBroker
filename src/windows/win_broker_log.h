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

#ifndef WIN_BROKER_LOG_H_
#define WIN_BROKER_LOG_H_

#include "rdmnet/broker/log.h"

#include <cstdio>
#include <fstream>
#include <winsock2.h>
#include <windows.h>

class WindowsBrokerLog : public rdmnet::BrokerLog
{
private:
  FILE* log_file_{nullptr};

  bool OnStartup() override;
  void OnShutdown() override;
  void GetTimeFromCallback(EtcPalLogTimeParams& time) override;
  void OutputLogMsg(const std::string& str) override;

  std::wstring GetOrCreateLogFilePath();
  void RotateLogs(const std::wstring& log_file_path);
};

#endif  // WIN_BROKER_LOG_H_
