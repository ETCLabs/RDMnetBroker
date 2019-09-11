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

#include "broker_config.h"

#include <fstream>

constexpr char* kDefaultSettings = R"(
  {
    "cid": "",
    "uid_type": "dynamic",
    "uid": "",

    "dns_sd": {
      "service_instance_name": "",
      "manufacturer": "ETC",
      "model": "RDMnet Broker",
      "scope": ""
    },

    "max_connections": 0,
    "max_controllers": 0,
    "max_controller_messages": 500,
    "max_devices": 0,
    "max_device_messages": 500,
    "max_reject_connections": 1000
  }
)";

BrokerConfig::BrokerConfig()
{
  defaults_ = json::parse(kDefaultSettings);
}

BrokerConfig::ParseResult BrokerConfig::ReadFromFile(const std::string& file_name)
{
  std::ifstream file_stream(file_name);
  if (!file_stream.is_open())
    return ParseResult::kFileOpenErr;

  return ReadFromStream(file_stream);
}

BrokerConfig::ParseResult BrokerConfig::ReadFromStream(std::istream& stream)
{
  try
  {
    stream >> current_;
    return Validate(current_, defaults_);
  }
  catch (json::parse_error)
  {
    return ParseResult::kJsonParseErr;
  }
}

BrokerConfig::ParseResult BrokerConfig::Validate(const json& current_obj, const json& default_obj)
{
  for (auto item = current_obj.begin(); item != current_obj.end(); ++item)
  {
    // Check each key that matches an item in our default settings object.
    if (default_obj.contains(item.key()))
    {
      if (default_obj[item.key()].type() != item.value().type())
      {
        // The value type of this item does not match the type of the corresponding default setting.
        return ParseResult::kInvalidSetting;
      }

      // Recurse into sub-objects.
      if (item.value().is_object())
      {
        ParseResult sub_result = Validate(item.value(), default_obj[item.key()]);
        if (sub_result != ParseResult::kOk)
          return sub_result;
      }
    }
  }
  return ParseResult::kOk;
}
