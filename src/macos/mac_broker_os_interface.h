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

#ifndef WIN_BROKER_OS_INTERFACE_H_
#define WIN_BROKER_OS_INTERFACE_H_

#include "broker_os_interface.h"

class MacBrokerOsInterface final : public BrokerOsInterface
{
public:
  MacBrokerOsInterface() = default;
  ~MacBrokerOsInterface();

  // BrokerOsInterface
  std::string                           GetLogFilePath() const override;
  bool                                  OpenLogFile() override;
  std::pair<std::string, std::ifstream> GetConfFile(etcpal::Logger& log) override;

  // etcpal::LogMessageHandler
  etcpal::LogTimestamp GetLogTimestamp() override;
  void                 HandleLogMessage(const EtcPalLogStrings& strings) override;

private:
  std::ofstream log_stream_;
};

#endif  // WIN_BROKER_OS_INTERFACE_H_
