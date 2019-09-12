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

#define NOMINMAX
#include "broker_config.h"

#include <functional>
#include <fstream>
#include <limits>
#include <map>
#include "etcpal/uuid.h"

// A set of information needed to validate an element in the config file's JSON.
struct Validator
{
  // Pointer to the key
  const json::json_pointer pointer;
  // The expected value type
  const json::value_t type;
  // A function to validate and store the value in the config's settings structure.
  std::function<bool(const json&, BrokerConfig&)> validate_and_store;
  // A function to store the default setting in the settings structure if the value is not present
  // in the config file.
  std::function<void(BrokerConfig&)> store_default;
};

// The CID must be a string representation of a UUID.
bool ValidateAndStoreCid(const json& val, BrokerConfig& config)
{
  std::string str_val = val;
  return etcpal_string_to_uuid(&config.settings.cid, str_val.c_str(), str_val.size());
}

// The UID takes the form:
// "uid": {
//   "type": < "static" | "dynamic" >,
//   "manufacturer_id": <number, always present>,
//   "device_id": <number, present only if type is "static">
// }
bool ValidateAndStoreUid(const json& val, BrokerConfig& config)
{
  if (val.contains("type") && val["type"].is_string() && val.contains("manufacturer_id") &&
      val["manufacturer_id"].is_number_integer())
  {
    std::string type = val["type"];
    int64_t manufacturer_id = val["manufacturer_id"];

    if (manufacturer_id > 0 && manufacturer_id < 0x8000)
    {
      if (type == "static" && val.contains("device_id") && val["device_id"].is_number_integer())
      {
        int64_t device_id = val["device_id"];
        if (device_id >= 0 && device_id <= 0xffffffff)
        {
          RdmUid static_uid;
          static_uid.manu = static_cast<uint16_t>(manufacturer_id);
          static_uid.id = static_cast<uint32_t>(device_id);
          config.settings.SetStaticUid(static_uid);
          return true;
        }
      }
      else if (type == "dynamic" && !val.contains("device_id"))
      {
        config.settings.SetDynamicUid(static_cast<uint16_t>(manufacturer_id));
        return true;
      }
    }
  }
  return false;
}

// Store a generic string from the "dns_sd": {} object.
template <size_t TruncationSize>
bool ValidateAndStoreDnsString(const json& val, std::string& string)
{
  std::string str_val = val;
  if (!str_val.empty())
  {
    string = str_val.substr(0, TruncationSize - 1);
    return true;
  }
  return false;
}

// Validate an arithmetic type and set it in the settings struct if it is within the valid range
// for its type.
template <class T>
bool SetIfInRange(const json& val, T& setting)
{
  int64_t int_val = val;
  if (int_val >= std::numeric_limits<T>::min() && int_val <= std::numeric_limits<T>::max())
  {
    setting = static_cast<T>(int_val);
    return true;
  }
  return false;
}

// A typical configuration file looks something like:
// {
//   "cid": "4958ac8f-cd5e-42cd-ab7e-9797b0efd3ac",
//   "uid": {
//     "type": "dynamic",
//     "manufacturer_id": 25972
//   },
//
//   "dns_sd": {
//     "service_instance_name": "My ETC RDMnet Broker",
//     "manufacturer": "ETC",
//     "model": "RDMnet Broker",
//     "scope": "default"
//   },
//
//   "max_connections": 20000,
//   "max_controllers": 1000,
//   "max_controller_messages": 500,
//   "max_devices": 20000,
//   "max_device_messages": 500,
//   "max_reject_connections": 1000
// }
// clang-format off
static const std::vector<Validator> kSettingsValidatorArray = {
  {
    "/cid"_json_pointer,
    json::value_t::string,
    ValidateAndStoreCid,
    [](auto& config) { config.settings.cid = config.default_cid(); },
  },
  {
    "/uid"_json_pointer,
    json::value_t::object,
    ValidateAndStoreUid,
    [](auto& config) { config.settings.SetDynamicUid(0x6574); }, // Set ETC's manufacturer ID
  },
  {
    "/dns_sd/service_instance_name"_json_pointer,
    json::value_t::string,
    [](const json& val, auto& config) { return ValidateAndStoreDnsString<E133_SERVICE_NAME_STRING_PADDED_LENGTH>(val, config.settings.disc_attributes.dns_service_instance_name); },
    [](auto& config)
    {
      char uuid_str_buf[ETCPAL_UUID_STRING_BYTES];
      etcpal_uuid_to_string(uuid_str_buf, &config.default_cid());
      // Add our CID to the service instance name, to help disambiguate
      config.settings.disc_attributes.dns_service_instance_name = std::string("ETC RDMnet Broker ") + uuid_str_buf;
    }
  },
  {
    "/dns_sd/manufacturer"_json_pointer,
    json::value_t::string,
    [](const json& val, auto& config) { return ValidateAndStoreDnsString<E133_MANUFACTURER_STRING_PADDED_LENGTH>(val, config.settings.disc_attributes.dns_manufacturer); },
    [](auto& config) { config.settings.disc_attributes.dns_manufacturer = "ETC"; }
  },
  {
    "/dns_sd/model"_json_pointer,
    json::value_t::string,
    [](const json& val, auto& config) { return ValidateAndStoreDnsString<E133_MODEL_STRING_PADDED_LENGTH>(val, config.settings.disc_attributes.dns_model); },
    [](auto& config) { config.settings.disc_attributes.dns_model = "RDMnet Broker Service"; }
  },
  {
    "/dns_sd/scope"_json_pointer,
    json::value_t::string,
    [](const json& val, auto& config) { return ValidateAndStoreDnsString<E133_SCOPE_STRING_PADDED_LENGTH>(val, config.settings.disc_attributes.scope); },
    std::function<void(BrokerConfig&)>() // Leave the default constructed scope value.
  },
  {
    "/max_connections"_json_pointer,
    json::value_t::number_unsigned,
    [](const json& val, auto& config) { return SetIfInRange<unsigned int>(val, config.settings.max_connections); },
    std::function<void(BrokerConfig&)>() // Leave the default constructed value in the settings struct
  },
  {
    "/max_controllers"_json_pointer,
    json::value_t::number_unsigned,
    [](const json& val, auto& config) { return SetIfInRange<unsigned int>(val, config.settings.max_controllers); },
    std::function<void(BrokerConfig&)>() // Leave the default constructed value in the settings struct
  },
  {
    "/max_controller_messages"_json_pointer,
    json::value_t::number_unsigned,
    [](const json& val, auto& config) { return SetIfInRange<unsigned int>(val, config.settings.max_controller_messages); },
    std::function<void(BrokerConfig&)>() // Leave the default constructed value in the settings struct
  },
  {
    "/max_devices"_json_pointer,
    json::value_t::number_unsigned,
    [](const json& val, auto& config) { return SetIfInRange<unsigned int>(val, config.settings.max_devices); },
    std::function<void(BrokerConfig&)>() // Leave the default constructed value in the settings struct
  },
  {
    "/max_device_messages"_json_pointer,
    json::value_t::number_unsigned,
    [](const json& val, auto& config) { return SetIfInRange<unsigned int>(val, config.settings.max_device_messages); },
    std::function<void(BrokerConfig&)>() // Leave the default constructed value in the settings struct
  },
  {
    "/max_reject_connections"_json_pointer,
    json::value_t::number_unsigned,
    [](const json& val, auto& config) { return SetIfInRange<unsigned int>(val, config.settings.max_reject_connections); },
    std::function<void(BrokerConfig&)>() // Leave the default constructed value in the settings struct
  }
};
// clang-format on

// Read the JSON configuration from a file.
BrokerConfig::ParseResult BrokerConfig::ReadFromFile(const std::string& file_name)
{
  std::ifstream file_stream(file_name);
  if (!file_stream.is_open())
    return ParseResult::kFileOpenErr;

  return ReadFromStream(file_stream);
}

// Read the JSON configuration from an input stream.
BrokerConfig::ParseResult BrokerConfig::ReadFromStream(std::istream& stream)
{
  try
  {
    stream >> current_;
    return ValidateCurrent();
  }
  catch (json::parse_error)
  {
    return ParseResult::kJsonParseErr;
  }
}

// Validate the JSON object contained in the "current_" member, which presumably has just been
// deserialized. Currently, extra keys present in the JSON which we don't recognize are considered
// valid.
BrokerConfig::ParseResult BrokerConfig::ValidateCurrent()
{
  for (auto setting : kSettingsValidatorArray)
  {
    try
    {
      // Check each key that matches an item in our settings array.
      if (current_.contains(setting.pointer))
      {
        json val = current_[setting.pointer];

        // If a setting is set to "null" in the JSON, store the default value.
        if (val.is_null() && setting.store_default)
        {
          setting.store_default(*this);
        }
        else
        {
          if (val.type() != setting.type)
          {
            // The value type of this item does not match the type of the corresponding default setting.
            return ParseResult::kInvalidSetting;
          }

          // Try to validate the setting's value.
          if (!setting.validate_and_store(val, *this))
          {
            return ParseResult::kInvalidSetting;
          }
        }
      }
      else if (setting.store_default)
      {
        // The "store_default" function may not be present, in which case the default-constructed
        // value in the output settings is left as-is.
        setting.store_default(*this);
      }
    }
    catch (json::parse_error)
    {
      return ParseResult::kInvalidSetting;
    }
  }
  return ParseResult::kOk;
}
