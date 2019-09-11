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
#include "rdmnet/broker.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

class BrokerConfig
{
public:
  BrokerConfig();

  enum class ParseResult
  {
    kFileOpenErr,
    kJsonParseErr,
    kInvalidSetting,
    kOk
  };

  ParseResult ReadFromFile(const std::string& file_name);
  ParseResult ReadFromStream(std::istream& stream);
  rdmnet::BrokerSettings get() const { return settings_; }

private:
  ParseResult Validate(const json& current_obj, const json& default_obj);

  json defaults_;
  json current_;
  rdmnet::BrokerSettings settings_;
};

#endif  // _BROKER_CONFIG_H_
