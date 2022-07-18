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

#ifndef BROKER_OS_INTERFACE_H_
#define BROKER_OS_INTERFACE_H_

#include <fstream>
#include <string>
#include <utility>
#include "etcpal/cpp/log.h"
#include "broker_config.h"

class BrokerOsInterface : public etcpal::LogMessageHandler
{
public:
  virtual ~BrokerOsInterface() = default;

  virtual std::string                           GetLogFilePath() const = 0;
  virtual bool                                  OpenLogFile() = 0;
  virtual std::pair<std::string, std::ifstream> GetConfFile(etcpal::Logger& log) = 0;
};

#endif  // BROKER_OS_INTERFACE_H_
