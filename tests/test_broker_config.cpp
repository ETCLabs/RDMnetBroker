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

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <utility>
#include "gtest/gtest.h"

const std::string kTestUuid = "4958ac8f-cd5e-42cd-ab7e-9797b0efd3ac";

class TestBrokerConfig : public testing::Test
{
protected:
  void TestInvalidUnsignedIntValueHelper(const std::string& key);
  void TestValidUnsignedIntValueHelper(const std::string&                                           key,
                                       std::function<unsigned int(const rdmnet::Broker::Settings&)> value_getter);

  void TestDnsSdInvalidStringValueHelper(const std::string& key);
  void TestDnsSdValidStringValueHelper(const std::string&                                          key,
                                       std::function<std::string(const rdmnet::Broker::Settings&)> value_getter);

  BrokerConfig config_;
};

// Make sure a full, valid example config with example values for all current settings is parsed
// correctly.
TEST_F(TestBrokerConfig, FullValidConfigParsedCorrectly)
{
  const uint16_t kDynamicUidManu = 25972;

  const std::string kDnsServiceInstanceName = "My ETC RDMnet Broker";
  const std::string kDnsManufacturer = "ETC";
  const std::string kDnsModel = "RDMnet Broker";

  const std::string              kScope = "default";
  const uint16_t                 kListenPort = 8888;
  const std::vector<std::string> kListenInterfaces = {"eth0", "eth1", "wlan0", "wlan1"};

  const unsigned int kMaxConnections = 20000;
  const unsigned int kMaxControllers = 1000;
  const unsigned int kMaxControllerMessages = 500;
  const unsigned int kMaxDevices = 20000;
  const unsigned int kMaxDeviceMessages = 500;
  const unsigned int kMaxRejectConnections = 1000;

  // clang-format off
  const std::string kFullValidConfig = R"(
    {
      "cid": ")" + kTestUuid + R"(",
      "uid": {
        "type": "dynamic",
        "manufacturer_id": )" + std::to_string(kDynamicUidManu) + R"(
      },

      "dns_sd": {
        "service_instance_name": ")" + kDnsServiceInstanceName + R"(",
        "manufacturer": ")" + kDnsManufacturer + R"(",
        "model": ")" + kDnsModel + R"("
      },

      "scope": ")" + kScope + R"(",
      "listen_port": )" + std::to_string(kListenPort) + R"(,
      "listen_interfaces": [ )" + std::accumulate(std::next(kListenInterfaces.begin()), kListenInterfaces.end(),
                                            "\"" + *kListenInterfaces.begin() + "\"",
                                            [](std::string a, auto b) {
                                              return std::move(a) + ", \"" + b + "\"";
                                            }) + R"( ],

      "log_level": "err",

      "max_connections": )" + std::to_string(kMaxConnections) + R"(,
      "max_controllers": )" + std::to_string(kMaxControllers) + R"(,
      "max_controller_messages": )" + std::to_string(kMaxControllerMessages) + R"(,
      "max_devices": )" + std::to_string(kMaxDevices) + R"(,
      "max_device_messages": )" + std::to_string(kMaxDeviceMessages) + R"(,
      "max_reject_connections": )" + std::to_string(kMaxRejectConnections) + R"(
    }
  )";
  // clang-format on

  std::istringstream test_stream(kFullValidConfig);
  ASSERT_EQ(config_.Read(test_stream), BrokerConfig::ParseResult::kOk);

  EXPECT_EQ(config_.settings.cid.ToString(), kTestUuid);

  EXPECT_TRUE(config_.settings.uid.IsDynamicUidRequest());
  EXPECT_EQ(config_.settings.uid.manufacturer_id(), kDynamicUidManu);

  EXPECT_EQ(config_.settings.dns.service_instance_name, kDnsServiceInstanceName);
  EXPECT_EQ(config_.settings.dns.manufacturer, kDnsManufacturer);
  EXPECT_EQ(config_.settings.dns.model, kDnsModel);

  EXPECT_EQ(config_.settings.scope, kScope);
  EXPECT_EQ(config_.settings.listen_port, kListenPort);
  EXPECT_EQ(config_.settings.listen_interfaces, kListenInterfaces);

  EXPECT_EQ(config_.log_mask, ETCPAL_LOG_UPTO(ETCPAL_LOG_ERR));

  EXPECT_EQ(config_.settings.limits.connections, kMaxConnections);
  EXPECT_EQ(config_.settings.limits.controllers, kMaxControllers);
  EXPECT_EQ(config_.settings.limits.controller_messages, kMaxControllerMessages);
  EXPECT_EQ(config_.settings.limits.devices, kMaxDevices);
  EXPECT_EQ(config_.settings.limits.device_messages, kMaxDeviceMessages);
  EXPECT_EQ(config_.settings.limits.reject_connections, kMaxRejectConnections);
}

TEST_F(TestBrokerConfig, InvalidJsonShouldFailWithoutThrowing)
{
  // We're not trying to replicate the (very comprehensive!) testing of our 3rd-party JSON library,
  // just feed our config parser a small set of invalid JSON values to make sure it handles errors
  // from the library correctly.

  // clang-format off
  const std::vector<std::string> kInvalidJsonStrings =
  {
    "",
    "{",
    "}",
    "\"unterminated_string",
    "[ \"object\": false ]",
    "{ bad_key: 20 }",
    "\xff\xff\xff\xff" // Invalid UTF-8
  };
  // clang-format on

  for (const auto& invalid_input : kInvalidJsonStrings)
  {
    std::istringstream test_stream(invalid_input);
    EXPECT_EQ(config_.Read(test_stream), BrokerConfig::ParseResult::kJsonParseErr) << "Input tested: " << invalid_input;
  }
}

TEST_F(TestBrokerConfig, InvalidCidValueShouldFail)
{
  // clang-format off
  const std::vector<std::string> kInvalidCidStrings =
  {
    // Invalid types
    R"( { "cid": 0 } )",
    R"( { "cid": false } )",
    R"( { "cid": true } )",
    R"( { "cid": {} } )",
    R"( { "cid": [] } )",
    // Invalid UUID formats
    R"( { "cid": "" } )",
    R"( { "cid": "monkey" } )"
  };
  // clang-format on

  for (const auto& invalid_input : kInvalidCidStrings)
  {
    std::istringstream test_stream(invalid_input);
    EXPECT_EQ(config_.Read(test_stream), BrokerConfig::ParseResult::kInvalidSetting)
        << "Input tested: " << invalid_input;
  }
}

TEST_F(TestBrokerConfig, CidCreatedIfNotPresentInConfig)
{
  const std::string kCidNotPresent = "{}";
  const std::string kCidIsNull = R"( { "cid": null } )";

  std::istringstream test_stream(kCidNotPresent);
  EXPECT_EQ(config_.Read(test_stream), BrokerConfig::ParseResult::kOk);

  EXPECT_FALSE(config_.settings.cid.IsNull());

  test_stream = std::istringstream(kCidIsNull);
  EXPECT_EQ(config_.Read(test_stream), BrokerConfig::ParseResult::kOk);
  EXPECT_FALSE(config_.settings.cid.IsNull());
}

TEST_F(TestBrokerConfig, ValidCidParsedCorrectly)
{
  const std::string  kValidCid = "1ef44b69-2185-4e3a-945f-a5a264c405e8";
  const std::string  kConfigContainingValidCid = R"( { "cid": ")" + kValidCid + R"( " } )";
  std::istringstream test_stream(kConfigContainingValidCid);
  EXPECT_EQ(config_.Read(test_stream), BrokerConfig::ParseResult::kOk);

  // The CID should be parsed
  EXPECT_EQ(config_.settings.cid, etcpal::Uuid::FromString(kValidCid));
}

TEST_F(TestBrokerConfig, InvalidUidValueShouldFail)
{
  // clang-format off
  const std::vector<std::string> kInvalidUidStrings =
  {
    // Invalid types
    R"( { "uid": 0 } )",
    R"( { "cid": "" } )",
    R"( { "uid": false } )",
    R"( { "uid": true } )",
    R"( { "uid": [] } )",
    // Invalid object formats
    R"( { "uid": {} } )",
    R"( { "uid": { "type": "dynamic" } } )",
    R"( { "uid": { "type": "static" } } )",
    R"( { "uid": { "type": "static", "manufacturer_id": 20 } } )",
    R"( { "uid": { "type": "static", "device_id": 30 } } )",
    R"( { "uid": { "type": "dynamic", "device_id": 30 } } )",
    R"( { "uid": { "type": "dynamic", "manufacturer_id": 20, "device_id": 30 } } )",
    // Invalid formats for "type"
    R"( { "uid": { "type": "blah" } } )",
    R"( { "uid": { "type": 0 } } )",
    R"( { "uid": { "type": true } } )",
    R"( { "uid": { "type": false } } )",
    R"( { "uid": { "type": {} } } )",
    R"( { "uid": { "type": [] } } )",
    R"( { "uid": { "type": null } } )",
    // Invalid formats for "manufacturer_id"
    R"( { "uid": { "type": "dynamic", "manufacturer_id": "20" } } )",
    R"( { "uid": { "type": "dynamic", "manufacturer_id": true } } )",
    R"( { "uid": { "type": "dynamic", "manufacturer_id": false } } )",
    R"( { "uid": { "type": "dynamic", "manufacturer_id": {} } } )",
    R"( { "uid": { "type": "dynamic", "manufacturer_id": [] } } )",
    R"( { "uid": { "type": "dynamic", "manufacturer_id": null } } )",
    // Invalid formats for "device_id"
    R"( { "uid": { "type": "static", "manufacturer_id": 20, "device_id": "30" } } )",
    R"( { "uid": { "type": "static", "manufacturer_id": 20, "device_id": true } } )",
    R"( { "uid": { "type": "static", "manufacturer_id": 20, "device_id": false } } )",
    R"( { "uid": { "type": "static", "manufacturer_id": 20, "device_id": {} } } )",
    R"( { "uid": { "type": "static", "manufacturer_id": 20, "device_id": [] } } )",
    R"( { "uid": { "type": "static", "manufacturer_id": 20, "device_id": null } } )",
    // ID values out of range
    R"( { "uid": { "type": "static", "manufacturer_id": 32768, "device_id": 30 } } )",
    R"( { "uid": { "type": "static", "manufacturer_id": 20.4, "device_id": 30 } } )",
    R"( { "uid": { "type": "static", "manufacturer_id": -1000, "device_id": 30 } } )",
    R"( { "uid": { "type": "static", "manufacturer_id": 20, "device_id": 4294967296 } } )",
    R"( { "uid": { "type": "static", "manufacturer_id": 20, "device_id": 10e+30 } } )",
    R"( { "uid": { "type": "static", "manufacturer_id": 20, "device_id": 30.4 } } )",
    R"( { "uid": { "type": "static", "manufacturer_id": 20, "device_id": -1000 } } )",
    R"( { "uid": { "type": "dynamic", "manufacturer_id": 32768 } } )"
    R"( { "uid": { "type": "dynamic", "manufacturer_id": 20.4 } } )",
    R"( { "uid": { "type": "dynamic", "manufacturer_id": -1000 } } )",
  };
  // clang-format on

  for (const auto& invalid_input : kInvalidUidStrings)
  {
    std::istringstream test_stream(invalid_input);
    EXPECT_EQ(config_.Read(test_stream), BrokerConfig::ParseResult::kInvalidSetting)
        << "Input tested: " << invalid_input;
  }
}

TEST_F(TestBrokerConfig, UidDynamicIfNotPresentInConfig)
{
  const std::string kUidNotPresent = "{}";
  const std::string kUidIsNull = R"( { "uid": null } )";

  std::istringstream test_stream(kUidNotPresent);
  EXPECT_EQ(config_.Read(test_stream), BrokerConfig::ParseResult::kOk);

  EXPECT_TRUE(config_.settings.uid.IsDynamicUidRequest());
  EXPECT_NE(config_.settings.uid.manufacturer_id(), 0x8000u);

  test_stream = std::istringstream(kUidIsNull);
  EXPECT_EQ(config_.Read(test_stream), BrokerConfig::ParseResult::kOk);
  EXPECT_TRUE(config_.settings.uid.IsDynamicUidRequest());
  EXPECT_NE(config_.settings.uid.manufacturer_id(), 0x8000u);
}

TEST_F(TestBrokerConfig, ValidUidParsedCorrectly)
{
  rdm::Uid          valid_static_uid = rdm::Uid::Static(16000, 3333333);
  const std::string kValidStaticUidConfig = R"( {
    "uid": {
      "type": "static",
      "manufacturer_id": 16000,
      "device_id": 3333333
    }
  } )";

  rdm::Uid          valid_dynamic_uid = rdm::Uid::DynamicUidRequest(17000);
  const std::string kValidDynamicUidConfig = R"( {
    "uid": {
      "type": "dynamic",
      "manufacturer_id": 17000
    }
  } )";

  std::istringstream test_stream(kValidStaticUidConfig);
  EXPECT_EQ(config_.Read(test_stream), BrokerConfig::ParseResult::kOk);

  // Correctly parsed static UID
  EXPECT_EQ(config_.settings.uid, valid_static_uid);

  test_stream = std::istringstream(kValidDynamicUidConfig);
  EXPECT_EQ(config_.Read(test_stream), BrokerConfig::ParseResult::kOk);

  // Correctly parsed dynamic UID
  EXPECT_EQ(config_.settings.uid, valid_dynamic_uid);
}

void TestBrokerConfig::TestDnsSdInvalidStringValueHelper(const std::string& key)
{
  // clang-format off
  const std::vector<std::string> kInvalidStrings =
  {
    // Invalid types
    R"( { "dns_sd": { ")" + key + R"(": 0 } } )",
    R"( { "dns_sd": { ")" + key + R"(": false } } )",
    R"( { "dns_sd": { ")" + key + R"(": true } } )",
    R"( { "dns_sd": { ")" + key + R"(": {} } } )",
    R"( { "dns_sd": { ")" + key + R"(": [] } } )",
    // Empty string is not valid
    R"( { "dns_sd": { ")" + key + R"(": "" } } )",
  };
  // clang-format on

  for (const auto& invalid_input : kInvalidStrings)
  {
    std::istringstream test_stream(invalid_input);
    EXPECT_EQ(config_.Read(test_stream), BrokerConfig::ParseResult::kInvalidSetting)
        << "Input tested: " << invalid_input;
  }
}

void TestBrokerConfig::TestDnsSdValidStringValueHelper(
    const std::string&                                          key,
    std::function<std::string(const rdmnet::Broker::Settings&)> value_getter)
{
  const std::string kTestString = "Broker String Name From Unit Tests";
  const std::string kValidConfig = R"( { "dns_sd": { ")" + key + R"(": ")" + kTestString + R"(" } } )";

  std::istringstream test_stream(kValidConfig);
  EXPECT_EQ(config_.Read(test_stream), BrokerConfig::ParseResult::kOk);
  EXPECT_EQ(value_getter(config_.settings), kTestString);
}

TEST_F(TestBrokerConfig, InvalidDnsSdServiceInstanceNameShouldFail)
{
  TestDnsSdInvalidStringValueHelper("service_instance_name");
}

TEST_F(TestBrokerConfig, ValidDnsSdServiceInstanceNameParsedCorrectly)
{
  TestDnsSdValidStringValueHelper("service_instance_name",
                                  [](const auto& settings) { return settings.dns.service_instance_name; });
}

TEST_F(TestBrokerConfig, InvalidDnsSdManufacturerShouldFail)
{
  TestDnsSdInvalidStringValueHelper("manufacturer");
}

TEST_F(TestBrokerConfig, ValidDnsSdManufacturerParsedCorrectly)
{
  TestDnsSdValidStringValueHelper("manufacturer", [](const auto& settings) { return settings.dns.manufacturer; });
}

TEST_F(TestBrokerConfig, InvalidDnsSdModelShouldFail)
{
  TestDnsSdInvalidStringValueHelper("model");
}

TEST_F(TestBrokerConfig, ValidDnsSdModelParsedCorrectly)
{
  TestDnsSdValidStringValueHelper("model", [](const auto& settings) { return settings.dns.model; });
}

TEST_F(TestBrokerConfig, InvalidScopeShouldFail)
{
  // clang-format off
  const std::vector<std::string> kInvalidStrings =
  {
    // Invalid types
    R"( { "scope": 0 } )",
    R"( { "scope": false } )",
    R"( { "scope": true } )",
    R"( { "scope": {} } )",
    R"( { "scope": [] } )",
    // Empty string is not valid
    R"( { "scope": "" } )",
    // Scope too long should fail
    // 63 octets is the max length, this is prescribed by the standard and unlikely to change
    //              |---------10--------20--------30--------40--------50--------60--------70
    R"( { "scope": "01234567890123456789012345678901234567890123456789012345678901234567890" } )"
  };
  // clang-format on

  for (const auto& invalid_input : kInvalidStrings)
  {
    std::istringstream test_stream(invalid_input);
    EXPECT_EQ(config_.Read(test_stream), BrokerConfig::ParseResult::kInvalidSetting)
        << "Input tested: " << invalid_input;
  }
}

TEST_F(TestBrokerConfig, ValidScopeParsedCorrectly)
{
  const std::string kTestScope = "Broker Scope From Unit Tests";
  const std::string kValidConfig = R"( { "scope": ")" + kTestScope + R"(" } )";

  std::istringstream test_stream(kValidConfig);
  EXPECT_EQ(config_.Read(test_stream), BrokerConfig::ParseResult::kOk);
  EXPECT_EQ(config_.settings.scope, kTestScope);
}

TEST_F(TestBrokerConfig, InvalidListenPortShouldFail)
{
  // clang-format off
  const std::vector<std::string> kInvalidStrings =
  {
    // Invalid types
    R"( { "listen_port": false } )",
    R"( { "listen_port": true } )",
    R"( { "listen_port": {} } )",
    R"( { "listen_port": [] } )",
    R"( { "listen_port": "string" } )",
    // Invalid values
    R"( { "listen_port": -20 } )",
    R"( { "listen_port": 0 } )",
    R"( { "listen_port": 1023 } )",
    R"( { "listen_port": 65536 } )",
  };
  // clang-format on

  for (const auto& invalid_input : kInvalidStrings)
  {
    std::istringstream test_stream(invalid_input);
    EXPECT_EQ(config_.Read(test_stream), BrokerConfig::ParseResult::kInvalidSetting)
        << "Input tested: " << invalid_input;
  }
}

TEST_F(TestBrokerConfig, ValidListenPortParsedCorrectly)
{
  // clang-format off
  const std::vector<std::pair<std::string, uint16_t>> kValidPortStrings =
  {
    { R"( { "listen_port": 1024 } )", static_cast<uint16_t>(1024) },
    { R"( { "listen_port": 8888 } )", static_cast<uint16_t>(8888) },
    { R"( { "listen_port": 65535 } )", static_cast<uint16_t>(65535) }
  };
  // clang-format on

  for (const auto& valid_input : kValidPortStrings)
  {
    std::istringstream test_stream(valid_input.first);
    EXPECT_EQ(config_.Read(test_stream), BrokerConfig::ParseResult::kOk) << "Input tested: " << valid_input.first;
    EXPECT_EQ(config_.settings.listen_port, valid_input.second);
  }
}

TEST_F(TestBrokerConfig, InvalidInterfaceListShouldFail)
{
  // clang-format off
  const std::vector<std::string> kInvalidStrings =
  {
    // Invalid types
    R"( { "listen_interfaces": 0 } )",
    R"( { "listen_interfaces": false } )",
    R"( { "listen_interfaces": true } )",
    R"( { "listen_interfaces": {} } )",
    R"( { "listen_interfaces": "string" } )",
    // Invalid values
    R"( { "listen_interfaces": [ 0 ] } )",
    R"( { "listen_interfaces": [ false ] } )",
    R"( { "listen_interfaces": [ true ] } )",
    R"( { "listen_interfaces": [ { "iface": "eth0" } ] } )",
    R"( { "listen_interfaces": [ [ "eth0" ] ] } )",
    R"( { "listen_interfaces": [ "eth0", 20, "wlan0" ] } )"
  };
  // clang-format on

  for (const auto& invalid_input : kInvalidStrings)
  {
    std::istringstream test_stream(invalid_input);
    EXPECT_EQ(config_.Read(test_stream), BrokerConfig::ParseResult::kInvalidSetting)
        << "Input tested: " << invalid_input;
  }
}

TEST_F(TestBrokerConfig, ValidInterfaceListParsedSuccessfully)
{
  const std::vector<std::string> kTestInterfaceList = {"eth0", "wlan0"};
  const std::string kValidConfig = R"( { "listen_interfaces": [ ")" + *kTestInterfaceList.begin() + R"(", ")" +
                                   *std::next(kTestInterfaceList.begin()) + R"(" ] } )";

  std::istringstream test_stream(kValidConfig);
  ASSERT_EQ(config_.Read(test_stream), BrokerConfig::ParseResult::kOk);
  EXPECT_EQ(config_.settings.listen_interfaces, kTestInterfaceList);
}

TEST_F(TestBrokerConfig, InvalidLogLevelShouldFail)
{
  // clang-format off
  const std::vector<std::string> kInvalidStrings =
  {
    // Invalid types
    R"( { "log_level": 0 } )",
    R"( { "log_level": false } )",
    R"( { "log_level": true } )",
    R"( { "log_level": {} } )",
    R"( { "log_level": [] } )",
    // Invalid values
    R"( { "log_level": "blah" } )",
    R"( { "log_level": "" } )",
  };
  // clang-format on

  for (const auto& invalid_input : kInvalidStrings)
  {
    std::istringstream test_stream(invalid_input);
    EXPECT_EQ(config_.Read(test_stream), BrokerConfig::ParseResult::kInvalidSetting)
        << "Input tested: " << invalid_input;
  }
}

TEST_F(TestBrokerConfig, ValidLogLevelParsedCorrectly)
{
  // clang-format off
  const std::vector<std::pair<std::string, int>> kValidLogLevelStrings =
  {
    { R"( { "log_level": "emerg" } )", ETCPAL_LOG_UPTO(ETCPAL_LOG_EMERG) },
    { R"( { "log_level": "debug" } )", ETCPAL_LOG_UPTO(ETCPAL_LOG_DEBUG) },
  };
  // clang-format on

  for (const auto& valid_input : kValidLogLevelStrings)
  {
    std::istringstream test_stream(valid_input.first);
    EXPECT_EQ(config_.Read(test_stream), BrokerConfig::ParseResult::kOk) << "Input tested: " << valid_input.first;
    EXPECT_EQ(config_.log_mask, valid_input.second);
  }
}

void TestBrokerConfig::TestInvalidUnsignedIntValueHelper(const std::string& key)
{
  // clang-format off
  const std::vector<std::string> kInvalidIntStrings = {
    R"( { ")" + key + R"(": false } )",
    R"( { ")" + key + R"(": true } )",
    R"( { ")" + key + R"(": {} } )",
    R"( { ")" + key + R"(": [] } )",
    R"( { ")" + key + R"(": "string" } )",
    R"( { ")" + key + R"(": -1000 })",
    R"( { ")" + key + R"(": -30.3 })",
    R"( { ")" + key + R"(": 20.3 })",
    // This one might be a bit brittle.... it will fail if sizeof(unsigned long long) == sizeof(unsigned int)...
    // Is this possible? Better ideas?
    R"( { ")" + key + R"(": )" + std::to_string(static_cast<unsigned long long>(std::numeric_limits<unsigned int>::max()) + 1) + " }",
  };
  // clang-format on

  for (const auto& invalid_input : kInvalidIntStrings)
  {
    std::istringstream test_stream(invalid_input);
    EXPECT_EQ(config_.Read(test_stream), BrokerConfig::ParseResult::kInvalidSetting)
        << "Input tested: " << invalid_input;
  }
}

void TestBrokerConfig::TestValidUnsignedIntValueHelper(
    const std::string&                                           key,
    std::function<unsigned int(const rdmnet::Broker::Settings&)> value_getter)
{
  // clang-format off
  const std::vector<std::pair<std::string, unsigned int>> kValidIntStrings = {
    { R"( { ")" + key + R"(": 0 })", 0 },
    { R"( { ")" + key + R"(": 1000 })", 1000 },
    {
      R"( { ")" + key + R"(": )" + std::to_string(std::numeric_limits<unsigned int>::max()) + " }",
      std::numeric_limits<unsigned int>::max()
    }
  };
  // clang-format on

  for (const auto& valid_input : kValidIntStrings)
  {
    std::istringstream test_stream(valid_input.first);
    EXPECT_EQ(config_.Read(test_stream), BrokerConfig::ParseResult::kOk) << "Input tested: " << valid_input.first;
    EXPECT_EQ(value_getter(config_.settings), valid_input.second);
  }
}

TEST_F(TestBrokerConfig, InvalidMaxConnectionsValueShouldFail)
{
  TestInvalidUnsignedIntValueHelper("max_connections");
}

TEST_F(TestBrokerConfig, ValidMaxConnectionsParsedCorrectly)
{
  TestValidUnsignedIntValueHelper("max_connections", [](const auto& settings) { return settings.limits.connections; });
}

TEST_F(TestBrokerConfig, InvalidMaxControllersValueShouldFail)
{
  TestInvalidUnsignedIntValueHelper("max_controllers");
}

TEST_F(TestBrokerConfig, ValidMaxControllersParsedCorrectly)
{
  TestValidUnsignedIntValueHelper("max_controllers", [](const auto& settings) { return settings.limits.controllers; });
}

TEST_F(TestBrokerConfig, InvalidMaxControllerMessagesValueShouldFail)
{
  TestInvalidUnsignedIntValueHelper("max_controller_messages");
}

TEST_F(TestBrokerConfig, ValidMaxControllerMessagesParsedCorrectly)
{
  TestValidUnsignedIntValueHelper("max_controller_messages",
                                  [](const auto& settings) { return settings.limits.controller_messages; });
}

TEST_F(TestBrokerConfig, InvalidMaxDevicesValueShouldFail)
{
  TestInvalidUnsignedIntValueHelper("max_devices");
}

TEST_F(TestBrokerConfig, ValidMaxDevicesParsedCorrectly)
{
  TestValidUnsignedIntValueHelper("max_devices", [](const auto& settings) { return settings.limits.devices; });
}

TEST_F(TestBrokerConfig, InvalidMaxDeviceMessagesValueShouldFail)
{
  TestInvalidUnsignedIntValueHelper("max_device_messages");
}

TEST_F(TestBrokerConfig, ValidMaxDeviceMessagesParsedCorrectly)
{
  TestValidUnsignedIntValueHelper("max_device_messages",
                                  [](const auto& settings) { return settings.limits.device_messages; });
}

TEST_F(TestBrokerConfig, InvalidMaxRejectConnectionsValueShouldFail)
{
  TestInvalidUnsignedIntValueHelper("max_reject_connections");
}

TEST_F(TestBrokerConfig, ValidMaxRejectConnectionsParsedCorrectly)
{
  TestValidUnsignedIntValueHelper("max_reject_connections",
                                  [](const auto& settings) { return settings.limits.reject_connections; });
}

TEST_F(TestBrokerConfig, SetDefaultsRestoresDefaultsConsistently)
{
  // Generate defaults to compare against later from a freshly-constructed config
  config_ = BrokerConfig();
  config_.SetDefaults();
  auto initial_defaults = config_;

  // Change a good number of settings from their defaults
  config_.enable_broker = false;
  config_.settings.cid = etcpal::Uuid::FromString(kTestUuid);
  config_.settings.uid = rdm::Uid(0x1234u, 0x56789012u);
  config_.settings.dns.service_instance_name = "TestSrvName";
  config_.settings.limits.connections = 1u;
  config_.settings.limits.controllers = 2u;
  config_.settings.limits.controller_messages = 3u;
  config_.settings.limits.devices = 4u;
  config_.settings.limits.device_messages = 5u;
  config_.settings.limits.reject_connections = 6u;
  config_.settings.scope = "test123";
  config_.settings.listen_port = 1234u;
  config_.settings.listen_interfaces.push_back(kTestUuid);

  // Now try restoring defaults again and verify they're the same as the original defaults
  config_.SetDefaults();

  EXPECT_EQ(config_.settings.cid, initial_defaults.settings.cid);
  EXPECT_EQ(config_.settings.uid, initial_defaults.settings.uid);
  EXPECT_EQ(config_.settings.dns.service_instance_name, initial_defaults.settings.dns.service_instance_name);
  EXPECT_EQ(config_.settings.limits.connections, initial_defaults.settings.limits.connections);
  EXPECT_EQ(config_.settings.limits.controllers, initial_defaults.settings.limits.controllers);
  EXPECT_EQ(config_.settings.limits.controller_messages, initial_defaults.settings.limits.controller_messages);
  EXPECT_EQ(config_.settings.limits.devices, initial_defaults.settings.limits.devices);
  EXPECT_EQ(config_.settings.limits.device_messages, initial_defaults.settings.limits.device_messages);
  EXPECT_EQ(config_.settings.limits.reject_connections, initial_defaults.settings.limits.reject_connections);
  EXPECT_EQ(config_.settings.scope, initial_defaults.settings.scope);
  EXPECT_EQ(config_.settings.listen_port, initial_defaults.settings.listen_port);
  EXPECT_EQ(config_.settings.listen_interfaces, initial_defaults.settings.listen_interfaces);
  EXPECT_EQ(config_.log_mask, initial_defaults.log_mask);
  EXPECT_EQ(config_.enable_broker, initial_defaults.enable_broker);
}
