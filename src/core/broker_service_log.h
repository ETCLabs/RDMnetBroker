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

#ifndef BROKER_SERVICE_LOG_H_
#define BROKER_SERVICE_LOG_H_

#include "rdmnet/broker/log.h"
#include "broker_os_interface.h"

class BrokerServiceLog
{
public:
  bool Startup(BrokerOsInterface& os_interface);
  void Shutdown();

  std::wstring GetOrCreateLogFilePath();
  void RotateLogs(const std::wstring& log_file_path);

private:
  rdmnet::BrokerLog log_;
};

#endif  // BROKER_SERVICE_LOG_H_
