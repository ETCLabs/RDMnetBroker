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

#include "broker_config.h"

#include <cinttypes>
#include <functional>
#include <fstream>
#include <limits>
#include <map>
#include <type_traits>
#include <utility>
#include "etcpal/uuid.h"
#include "broker_common.h"

constexpr const char kValidationFailLogPrefix[] = "Invalid value found in configuration file: ";
constexpr const char kValidationFailLogPostfix[] = " (default will be used instead).";

template <typename... Args>
void LogParseError(etcpal::Logger* log, const std::string& format, Args&&... args)
{
  if (log)
  {
    log->Notice(std::string(kValidationFailLogPrefix + format + kValidationFailLogPostfix).c_str(),
                std::forward<Args>(args)...);
  }
}

// A set of information needed to validate an element in the config file's JSON.
struct Validator final
{
  // Pointer to the key
  const json::json_pointer pointer;
  // The expected value type
  const json::value_t type;
  // A function to validate and store the value in the config's settings structure. This must be provided.
  std::function<bool(const json&, BrokerConfig&, etcpal::Logger*)> validate_and_store;
  // A function to store the default setting in the settings structure if the value is not present
  // in the config file or is invalid. This must be provided.
  std::function<void(BrokerConfig&)> store_default;
};

// The CID must be a string representation of a UUID.
bool ValidateAndStoreCid(const json& val, BrokerConfig& config, etcpal::Logger* log)
{
  config.settings.cid = etcpal::Uuid::FromString(val);
  if (!config.settings.cid.IsNull())
  {
    return true;
  }
  else
  {
    LogParseError(log, "\"%s\" is not a valid CID", std::string(val).c_str());
    return false;
  }
}

// The UID takes the form:
// "uid": {
//   "type": < "static" | "dynamic" >,
//   "manufacturer_id": <number, always present>,
//   "device_id": <number, present only if type is "static">
// }
bool ValidateAndStoreUid(const json& val, BrokerConfig& config, etcpal::Logger* log)
{
  if (!val.contains("type"))
  {
    LogParseError(log, "The \"uid\" object must contain a \"type\" field");
    return false;
  }
  if (!val["type"].is_string())
  {
    LogParseError(log, "The value for setting \"/uid/type\" was of invalid type \"%s\"", val["type"].type_name());
    return false;
  }
  if (!val.contains("manufacturer_id"))
  {
    LogParseError(log, "The \"uid\" object must contain a \"manufacturer_id\" field");
    return false;
  }
  if (!val["manufacturer_id"].is_number_integer())
  {
    LogParseError(log, "The value for setting \"/uid/manufacturer_id\" was of invalid type \"%s\"",
                  val["manufacturer_id"].type_name());
    return false;
  }

  const std::string type = val["type"];
  const int64_t     manufacturer_id = val["manufacturer_id"];

  if (manufacturer_id <= 0 || manufacturer_id >= 0x8000)
  {
    LogParseError(log, "UID: \"%" PRId64 "\" is not a valid Manufacturer ID", manufacturer_id);
    return false;
  }

  if (type == "static")
  {
    if (!val.contains("device_id"))
    {
      LogParseError(log, "When \"/uid/type\" is \"static\", the \"uid\" object must contain a \"device_id\" field");
      return false;
    }
    if (!val["device_id"].is_number_integer())
    {
      LogParseError(log, "The value for setting \"/uid/device_id\" was of invalid type \"%s\"",
                    val["device_id"].type_name());
      return false;
    }

    const int64_t device_id = val["device_id"];
    if (device_id < 0 || device_id > 0xffffffff)
    {
      LogParseError(log, "Static UID: \"%" PRId64 "\" is not a valid Device ID", device_id);
      return false;
    }

    config.settings.uid = rdm::Uid::Static(static_cast<uint16_t>(manufacturer_id), static_cast<uint32_t>(device_id));
    return true;
  }
  else if (type == "dynamic")
  {
    if (val.contains("device_id"))
    {
      LogParseError(log,
                    "When \"/uid/type\" is \"dynamic\", the \"uid\" object must not contain a \"device_id\" field");
      return false;
    }
    config.settings.uid = rdm::Uid::DynamicUidRequest(static_cast<uint16_t>(manufacturer_id));
    return true;
  }
  else
  {
    LogParseError(log, "The field \"/uid/type\" must be one of \"static\" or \"dynamic\"");
    return false;
  }
}

// Store a generic string.
bool ValidateAndStoreString(const char*     key_ptr,
                            const json&     val,
                            std::string&    string,
                            size_t          max_size,
                            etcpal::Logger* log,
                            bool            truncation_allowed = true)
{
  if (!BROKER_ASSERT_VERIFY(key_ptr, log))
    return false;

  const std::string str_val = val;
  if (str_val.empty())
  {
    LogParseError(log, "Empty string is not allowed for field \"%s\"", key_ptr);
    return false;
  }

  if (str_val.length() > max_size)
  {
    if (truncation_allowed)
    {
      string = str_val.substr(0, max_size);
      if (log)
      {
        log->Notice("Configuration file: Truncating overlong string \"%s\" for field \"%s\" to \"%s\".",
                    str_val.c_str(), key_ptr, string.c_str());
      }
      return true;
    }
    else
    {
      LogParseError(log, "String value \"%s\" is too long for field \"%s\" of maximum length %zu", str_val.c_str(),
                    key_ptr, max_size);
      return false;
    }
  }

  string = str_val;
  return true;
}

// Get the printf-style format string of a given integral type
template <typename IntType>
class FormatStringOf
{
public:
  static const char* value;
};

// Specialize this for each type that ValidateAndStoreInt is used for.
// clang-format off
template<> const char* FormatStringOf<unsigned int>::value = "%u";
template<> const char* FormatStringOf<uint16_t>::value = "%" PRIu16;
// clang-format on

// Validate an arithmetic type and set it in the settings struct if it is within the valid range
// for its type.
template <typename IntType>
bool ValidateAndStoreInt(
    const char*                        key_ptr,
    const json&                        val,
    IntType&                           setting,
    etcpal::Logger*                    log,
    const std::pair<IntType, IntType>& limits = std::make_pair<IntType, IntType>(std::numeric_limits<IntType>::min(),
                                                                                 std::numeric_limits<IntType>::max()))
{
  static_assert(std::is_integral<IntType>(), "This function can only be used with integral types.");

  if (!BROKER_ASSERT_VERIFY(key_ptr, log))
    return false;

  const int64_t int_val = val;
  if (int_val < limits.first || int_val > limits.second)
  {
    LogParseError(log,
                  std::string("Integer value \"%" PRId64 "\" is outside allowable range [") +
                      FormatStringOf<IntType>::value + ", " + FormatStringOf<IntType>::value + "] for field \"%s\"",
                  int_val, limits.first, limits.second, key_ptr);
    return false;
  }
  setting = static_cast<IntType>(int_val);
  return true;
}

bool ValidateAndStoreInterfaceList(const json& val, BrokerConfig& config, etcpal::Logger* log)
{
  const std::vector<json> listen_interfaces = val;
  for (const json& listen_interface : listen_interfaces)
  {
    if (listen_interface.type() != json::value_t::string)
    {
      LogParseError(log, "The array field \"/listen_interfaces\" may only contain values of type \"string\"");
      config.settings.listen_interfaces.clear();
      return false;
    }
    config.settings.listen_interfaces.push_back(listen_interface);
  }
  return true;
}

// clang-format off
const std::map<std::string, int> kLogLevelOptions = {
  {"debug", ETCPAL_LOG_UPTO(ETCPAL_LOG_DEBUG)},
  {"info", ETCPAL_LOG_UPTO(ETCPAL_LOG_INFO)},
  {"notice", ETCPAL_LOG_UPTO(ETCPAL_LOG_NOTICE)},
  {"warning", ETCPAL_LOG_UPTO(ETCPAL_LOG_WARNING)},
  {"err", ETCPAL_LOG_UPTO(ETCPAL_LOG_ERR)},
  {"crit", ETCPAL_LOG_UPTO(ETCPAL_LOG_CRIT)},
  {"alert", ETCPAL_LOG_UPTO(ETCPAL_LOG_ALERT)},
  {"emerg", ETCPAL_LOG_UPTO(ETCPAL_LOG_EMERG)},
};
// clang-format on

// Join all the possible log levels in a readable set of the form {"level1", "level2", ...} for
// error logging.
std::string GetLogLevelOptions()
{
  return "{" +
         std::accumulate(std::next(kLogLevelOptions.begin()), kLogLevelOptions.end(),
                         "\"" + kLogLevelOptions.begin()->first + "\"",
                         [](std::string a, auto b) { return std::move(a) + ", \"" + b.first + "\""; }) +
         "}";
}

bool ValidateAndStoreLogLevel(const json& val, BrokerConfig& config, etcpal::Logger* log)
{
  const std::string log_level = val;
  auto              level_pair = kLogLevelOptions.find(log_level);
  if (level_pair == kLogLevelOptions.end())
  {
    LogParseError(log, "The value for field \"/log_level\" must be one of " + GetLogLevelOptions());
    return false;
  }

  config.log_mask = level_pair->second;
  return true;
}

// A typical full, valid configuration file looks something like:
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
//   },
//
//   "scope": "default",
//   "listen_port": 8888,
//   "listen_interfaces": [
//     "eth0",
//     "wlan0"
//   ],
//
//   "log_level": "info",
//
//   "max_connections": 20000,
//   "max_controllers": 1000,
//   "max_controller_messages": 500,
//   "max_devices": 20000,
//   "max_device_messages": 500,
//   "max_reject_connections": 1000
// }
// Any or all of these items can be omitted to use the default value for that key.

// clang-format off
// This array should always be processed top to bottom in case the default of one property depends on the value of the property above it.
static const Validator kSettingsValidatorArray[] = {
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
    [](auto& config) { config.settings.uid = rdm::Uid::DynamicUidRequest(0x6574); }, // Set ETC's manufacturer ID
  },
  {
    "/dns_sd/service_instance_name"_json_pointer,
    json::value_t::string,
    [](const json& val, auto& config, auto log) {
      return ValidateAndStoreString("/dns_sd/service_instance_name", val, config.settings.dns.service_instance_name, E133_SERVICE_NAME_STRING_PADDED_LENGTH - 1, log);
    },
    [](auto& config) {
      // Add our CID to the service instance name, to help disambiguate.
      // This entry is placed after the CID property's entry in the array, so config.settings.cid has already been loaded and can be used.
      config.settings.dns.service_instance_name = "ETC RDMnet Broker " + config.settings.cid.ToString();
    }
  },
  {
    "/dns_sd/manufacturer"_json_pointer,
    json::value_t::string,
    [](const json& val, auto& config, auto log) {
      return ValidateAndStoreString("/dns_sd/manufacturer", val, config.settings.dns.manufacturer, E133_MANUFACTURER_STRING_PADDED_LENGTH - 1, log);
    },
    [](auto& config) { config.settings.dns.manufacturer = "ETC"; }
  },
  {
    "/dns_sd/model"_json_pointer,
    json::value_t::string,
    [](const json& val, auto& config, auto log) {
      return ValidateAndStoreString("/dns_sd/model", val, config.settings.dns.model, E133_MODEL_STRING_PADDED_LENGTH - 1, log);
    },
    [](auto& config) { config.settings.dns.model = "RDMnet Broker Service"; }
  },
  {
    "/scope"_json_pointer,
    json::value_t::string,
    [](const json& val, auto& config, auto log) {
      return ValidateAndStoreString("/scope", val, config.settings.scope, E133_SCOPE_STRING_PADDED_LENGTH -1, log, false);
    },
    [](auto& config) { config.settings.scope = E133_DEFAULT_SCOPE; }
  },
  {
    "/listen_port"_json_pointer,
    json::value_t::number_unsigned,
    [](const json& val, auto& config, auto log) {
      return ValidateAndStoreInt<uint16_t>("/listen_port", val, config.settings.listen_port, log, std::make_pair<uint16_t, uint16_t>(1024, 65535));
    },
    [](auto& config) { config.settings.listen_port = 0u; }
  },
  {
    "/listen_interfaces"_json_pointer,
    json::value_t::array,
    ValidateAndStoreInterfaceList,
    [](auto& config) { config.settings.listen_interfaces.clear(); }
  },
  {
    "/log_level"_json_pointer,
    json::value_t::string,
    ValidateAndStoreLogLevel,
    [](auto& config) { config.log_mask = ETCPAL_LOG_UPTO(ETCPAL_LOG_INFO); }
  },
  {
    "/max_connections"_json_pointer,
    json::value_t::number_unsigned,
    [](const json& val, auto& config, auto log) {
      return ValidateAndStoreInt<unsigned int>("/max_connections", val, config.settings.limits.connections, log);
    },
    [](auto& config) { config.settings.limits.connections = 0; }
  },
  {
    "/max_controllers"_json_pointer,
    json::value_t::number_unsigned,
    [](const json& val, auto& config, auto log) {
      return ValidateAndStoreInt<unsigned int>("/max_controllers", val, config.settings.limits.controllers, log);
    },
    [](auto& config) { config.settings.limits.controllers = 0; }
  },
  {
    "/max_controller_messages"_json_pointer,
    json::value_t::number_unsigned,
    [](const json& val, auto& config, auto log) {
      return ValidateAndStoreInt<unsigned int>("/max_controller_messages", val, config.settings.limits.controller_messages, log);
    },
    [](auto& config) { config.settings.limits.controller_messages = 500; }
  },
  {
    "/max_devices"_json_pointer,
    json::value_t::number_unsigned,
    [](const json& val, auto& config, auto log) {
      return ValidateAndStoreInt<unsigned int>("/max_devices", val, config.settings.limits.devices, log);
    },
    [](auto& config) { config.settings.limits.devices = 0; }
  },
  {
    "/max_device_messages"_json_pointer,
    json::value_t::number_unsigned,
    [](const json& val, auto& config, auto log) {
      return ValidateAndStoreInt<unsigned int>("/max_device_messages", val, config.settings.limits.device_messages, log);
    },
    [](auto& config) { config.settings.limits.device_messages = 500; }
  },
  {
    "/max_reject_connections"_json_pointer,
    json::value_t::number_unsigned,
    [](const json& val, auto& config, auto log) {
      return ValidateAndStoreInt<unsigned int>("/max_reject_connections", val, config.settings.limits.reject_connections, log);
    },
    [](auto& config) { config.settings.limits.reject_connections = 1000; }
  },
  {
    "/enable_broker"_json_pointer,
    json::value_t::boolean,
    [](const json& val, auto& config, auto log) {
      config.enable_broker = val;
      return true;
    },
    [](auto& config) { config.enable_broker = true; }
  }
};
// clang-format on

// Read the JSON configuration from an input stream.
BrokerConfig::ParseResult BrokerConfig::Read(std::istream& stream, etcpal::Logger* log)
{
  try
  {
    stream >> current_;
    return ValidateCurrent(log);
  }
  catch (json::parse_error& e)
  {
    if (log)
      log->Notice("Could not parse configuration file: %s", e.what());
    return ParseResult::kJsonParseErr;
  }
}

void BrokerConfig::SetDefaults()
{
  for (const auto& setting : kSettingsValidatorArray)  // kSettingsValidatorArray must be iterated first to last
  {
    if (setting.store_default)
      setting.store_default(*this);
  }
}

// Validate the JSON object contained in the "current_" member, which presumably has just been
// deserialized. Currently, extra keys present in the JSON which we don't recognize are considered
// valid.
BrokerConfig::ParseResult BrokerConfig::ValidateCurrent(etcpal::Logger* log)
{
  ParseResult res = ParseResult::kOk;

  for (const auto& setting : kSettingsValidatorArray)  // kSettingsValidatorArray must be iterated first to last
  {
    assert(setting.store_default);
    assert(setting.validate_and_store);

    // Check each key that matches an item in our settings array.
    if (current_.contains(setting.pointer))
    {
      const json val = current_[setting.pointer];

      // If a setting is set to "null" in the JSON, store the default value.
      if (val.is_null())
      {
        setting.store_default(*this);
      }
      else
      {
        if (val.type() != setting.type)
        {
          // The value type of this item does not match the type of the corresponding default setting.
          LogParseError(log, "The value for setting \"%s\" was of invalid type \"%s\"",
                        setting.pointer.to_string().c_str(), val.type_name());
          setting.store_default(*this);
          res = ParseResult::kInvalidSetting;
        }
        else if (!setting.validate_and_store(val, *this, log))  // Try to validate the setting's value.
        {
          setting.store_default(*this);
          res = ParseResult::kInvalidSetting;
        }
      }
    }
    else
    {
      // The "store_default" function may not be present, in which case the default-constructed
      // value in the output settings is left as-is.
      setting.store_default(*this);

      if (log)
      {
        log->Debug("Configuration file: No value present for \"%s\", using default.",
                   setting.pointer.to_string().c_str());
      }
    }
  }

  return res;
}
