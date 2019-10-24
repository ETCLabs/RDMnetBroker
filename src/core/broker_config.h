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

#ifndef BROKER_CONFIG_H_
#define BROKER_CONFIG_H_

#include <istream>
#include <string>
#include "etcpal/cpp/uuid.h"
#include "etcpal/inet.h"
#include "etcpal/log.h"
#include "etcpal/netint.h"
#include "rdmnet/broker.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

// A class to read the Broker's configuration file and translate it into the settings structure
// taken by the RDMnet Broker library.
class BrokerConfig
{
public:
  enum class ParseResult
  {
    kFileOpenErr,
    kJsonParseErr,
    kInvalidSetting,
    kOk
  };

  [[nodiscard]] ParseResult Read(std::istream& stream, rdmnet::BrokerLog* log = nullptr);
  void SetDefaults();

  [[nodiscard]] const etcpal::Uuid& default_cid() const { return default_cid_; }

  rdmnet::BrokerSettings settings;
  int log_mask{ETCPAL_LOG_UPTO(ETCPAL_LOG_INFO)};

private:
  ParseResult ValidateCurrent(rdmnet::BrokerLog* log);

  json current_;
  etcpal::Uuid default_cid_{etcpal::Uuid::OsPreferred()};
};

#endif  // BROKER_CONFIG_H_
