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

#ifndef _BROKER_CONFIG_H_
#define _BROKER_CONFIG_H_

#include <string>
#include <istream>
#include "etcpal/uuid.h"
#include "rdmnet/broker.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

// A class to read the Broker's configuration file and translate it into the settings structure
// taken by the RDMnet Broker library.
class BrokerConfig
{
public:
  // We generate a CID ahead of time, because we want to use the same one for both the Broker's CID,
  // and as a disambiguator in its DNS service instance name.
  BrokerConfig() { etcpal_generate_os_preferred_uuid(&default_cid_); }

  enum class ParseResult
  {
    kFileOpenErr,
    kJsonParseErr,
    kInvalidSetting,
    kOk
  };

  ParseResult Read(const std::string& file_name);
  ParseResult Read(std::istream& stream);

  rdmnet::BrokerSettings settings;

  const EtcPalUuid& default_cid() const { return default_cid_; }

private:
  ParseResult ValidateCurrent();

  json current_;
  EtcPalUuid default_cid_;
};

#endif  // _BROKER_CONFIG_H_
