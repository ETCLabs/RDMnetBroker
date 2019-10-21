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

#include <algorithm>
#include <sstream>
#include <iomanip>
#include "gtest/gtest.h"

class TestBrokerConfig : public testing::Test
{
protected:
  void TestInvalidUnsignedIntValueHelper(const std::string& key);
  void TestValidUnsignedIntValueHelper(const std::string& key,
                                       std::function<unsigned int(const rdmnet::BrokerSettings&)> value_getter);

  void TestDnsSdInvalidStringValueHelper(const std::string& key);
  void TestDnsSdValidStringValueHelper(const std::string& key,
                                       std::function<std::string(const rdmnet::BrokerSettings&)> value_getter);

  BrokerConfig config_;
};

// Make sure a full, valid example config with example values for all current settings is parsed
// correctly.
TEST_F(TestBrokerConfig, FullValidConfigParsedCorrectly)
{
  const std::string kCid = "4958ac8f-cd5e-42cd-ab7e-9797b0efd3ac";
  const uint16_t kDynamicUidManu = 25972;

  const std::string kDnsServiceInstanceName = "My ETC RDMnet Broker";
  const std::string kDnsManufacturer = "ETC";
  const std::string kDnsModel = "RDMnet Broker";

  const std::string kScope = "default";
  const uint16_t kListenPort = 8888;
  const std::set<rdmnet::BrokerSettings::MacAddress> kListenMacs = {{0x00, 0xc0, 0x16, 0x01, 0x23, 0x45},
                                                                    {0x00, 0xc0, 0x16, 0x33, 0x33, 0x33}};
  const std::set<EtcPalIpAddr> kListenAddrs;

  auto MacToString = [](const rdmnet::BrokerSettings::MacAddress& mac) {
    auto ByteToString = [](uint8_t byte) {
      std::stringstream stream;
      stream << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(byte);
      return stream.str();
    };
    return "\"" +
           std::accumulate(std::next(mac.begin()), mac.end(), ByteToString(mac[0]),
                           [&](std::string a, uint8_t b) { return std::move(a) + ':' + ByteToString(b); }) +
           "\"";
  };

  const unsigned int kMaxConnections = 20000;
  const unsigned int kMaxControllers = 1000;
  const unsigned int kMaxControllerMessages = 500;
  const unsigned int kMaxDevices = 20000;
  const unsigned int kMaxDeviceMessages = 500;
  const unsigned int kMaxRejectConnections = 1000;

  // clang-format off
  const std::string kFullValidConfig = R"(
    {
      "cid": ")" + kCid + R"(",
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
      "listen_macs": [ )" + std::accumulate(std::next(kListenMacs.begin()), kListenMacs.end(), MacToString(*kListenMacs.begin()), [MacToString](std::string a, auto b) {
          return std::move(a) + ", " + MacToString(b);
       }) + R"( ],

      "max_connections": )" + std::to_string(kMaxConnections) + R"(,
      "max_controllers": )" + std::to_string(kMaxControllers) + R"(,
      "max_controller_messages": )" + std::to_string(kMaxControllerMessages) + R"(,
      "max_devices": )" + std::to_string(kMaxDevices) + R"(,
      "max_device_messages": )" + std::to_string(kMaxDeviceMessages) + R"(,
      "max_reject_connections": )" + std::to_string(kMaxRejectConnections) + R"(
    }
  )";
  // clang-format on

  std::cout << kFullValidConfig;
  std::istringstream test_stream(kFullValidConfig);
  ASSERT_EQ(config_.Read(test_stream), BrokerConfig::ParseResult::kOk);

  EXPECT_EQ(config_.settings.cid.ToString(), kCid);

  EXPECT_EQ(config_.settings.uid_type, rdmnet::BrokerSettings::kDynamicUid);
  EXPECT_TRUE(RDMNET_UID_IS_DYNAMIC_UID_REQUEST(&config_.settings.uid));
  EXPECT_EQ(RDM_GET_MANUFACTURER_ID(&config_.settings.uid), kDynamicUidManu);

  EXPECT_EQ(config_.settings.dns.service_instance_name, kDnsServiceInstanceName);
  EXPECT_EQ(config_.settings.dns.manufacturer, kDnsManufacturer);
  EXPECT_EQ(config_.settings.dns.model, kDnsModel);

  EXPECT_EQ(config_.settings.scope, kScope);
  EXPECT_EQ(config_.settings.listen_port, kListenPort);

  EXPECT_EQ(config_.settings.max_connections, kMaxConnections);
  EXPECT_EQ(config_.settings.max_controllers, kMaxControllers);
  EXPECT_EQ(config_.settings.max_controller_messages, kMaxControllerMessages);
  EXPECT_EQ(config_.settings.max_devices, kMaxDevices);
  EXPECT_EQ(config_.settings.max_device_messages, kMaxDeviceMessages);
  EXPECT_EQ(config_.settings.max_reject_connections, kMaxRejectConnections);
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
  const std::string kValidCid = "1ef44b69-2185-4e3a-945f-a5a264c405e8";
  const std::string kConfigContainingValidCid = R"( { "cid": ")" + kValidCid + R"( " } )";
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

  EXPECT_EQ(config_.settings.uid_type, rdmnet::BrokerSettings::kDynamicUid);
  EXPECT_TRUE(RDMNET_UID_IS_DYNAMIC_UID_REQUEST(&config_.settings.uid));
  EXPECT_NE(config_.settings.uid.manu, 0x8000u);

  test_stream = std::istringstream(kUidIsNull);
  EXPECT_EQ(config_.Read(test_stream), BrokerConfig::ParseResult::kOk);
  EXPECT_EQ(config_.settings.uid_type, rdmnet::BrokerSettings::kDynamicUid);
  EXPECT_TRUE(RDMNET_UID_IS_DYNAMIC_UID_REQUEST(&config_.settings.uid));
  EXPECT_NE(config_.settings.uid.manu, 0x8000u);
}

TEST_F(TestBrokerConfig, ValidUidParsedCorrectly)
{
  RdmUid valid_static_uid = {16000, 3333333};
  const std::string kValidStaticUidConfig = R"( {
    "uid": {
      "type": "static",
      "manufacturer_id": 16000,
      "device_id": 3333333
    }
  } )";

  RdmUid valid_dynamic_uid;
  RDMNET_INIT_DYNAMIC_UID_REQUEST(&valid_dynamic_uid, 17000);
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
  EXPECT_EQ(config_.settings.uid_type, rdmnet::BrokerSettings::kStaticUid);

  test_stream = std::istringstream(kValidDynamicUidConfig);
  EXPECT_EQ(config_.Read(test_stream), BrokerConfig::ParseResult::kOk);

  // Correctly parsed dynamic UID
  EXPECT_EQ(config_.settings.uid, valid_dynamic_uid);
  EXPECT_EQ(config_.settings.uid_type, rdmnet::BrokerSettings::kDynamicUid);
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
    const std::string& key, std::function<std::string(const rdmnet::BrokerSettings&)> value_getter)
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

void TestBrokerConfig::TestInvalidUnsignedIntValueHelper(const std::string& key)
{
  // clang-format off
  const std::vector<std::string> kInvalidIntStrings = {
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
    const std::string& key, std::function<unsigned int(const rdmnet::BrokerSettings&)> value_getter)
{
  struct ValidInput
  {
    std::string config_string;
    unsigned int value;
  };

  // clang-format off
  const std::vector<ValidInput> kValidIntStrings = {
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
    std::istringstream test_stream(valid_input.config_string);
    EXPECT_EQ(config_.Read(test_stream), BrokerConfig::ParseResult::kOk)
        << "Input tested: " << valid_input.config_string;

    EXPECT_EQ(value_getter(config_.settings), valid_input.value);
  }
}

TEST_F(TestBrokerConfig, InvalidMaxConnectionsValueShouldFail)
{
  TestInvalidUnsignedIntValueHelper("max_connections");
}

TEST_F(TestBrokerConfig, ValidMaxConnectionsParsedCorrectly)
{
  TestValidUnsignedIntValueHelper("max_connections", [](const auto& settings) { return settings.max_connections; });
}

TEST_F(TestBrokerConfig, InvalidMaxControllersValueShouldFail)
{
  TestInvalidUnsignedIntValueHelper("max_controllers");
}

TEST_F(TestBrokerConfig, ValidMaxControllersParsedCorrectly)
{
  TestValidUnsignedIntValueHelper("max_controllers", [](const auto& settings) { return settings.max_controllers; });
}

TEST_F(TestBrokerConfig, InvalidMaxControllerMessagesValueShouldFail)
{
  TestInvalidUnsignedIntValueHelper("max_controller_messages");
}

TEST_F(TestBrokerConfig, ValidMaxControllerMessagesParsedCorrectly)
{
  TestValidUnsignedIntValueHelper("max_controller_messages",
                                  [](const auto& settings) { return settings.max_controller_messages; });
}

TEST_F(TestBrokerConfig, InvalidMaxDevicesValueShouldFail)
{
  TestInvalidUnsignedIntValueHelper("max_devices");
}

TEST_F(TestBrokerConfig, ValidMaxDevicesParsedCorrectly)
{
  TestValidUnsignedIntValueHelper("max_devices", [](const auto& settings) { return settings.max_devices; });
}

TEST_F(TestBrokerConfig, InvalidMaxDeviceMessagesValueShouldFail)
{
  TestInvalidUnsignedIntValueHelper("max_device_messages");
}

TEST_F(TestBrokerConfig, ValidMaxDeviceMessagesParsedCorrectly)
{
  TestValidUnsignedIntValueHelper("max_device_messages",
                                  [](const auto& settings) { return settings.max_device_messages; });
}

TEST_F(TestBrokerConfig, InvalidMaxRejectConnectionsValueShouldFail)
{
  TestInvalidUnsignedIntValueHelper("max_reject_connections");
}

TEST_F(TestBrokerConfig, ValidMaxRejectConnectionsParsedCorrectly)
{
  TestValidUnsignedIntValueHelper("max_reject_connections",
                                  [](const auto& settings) { return settings.max_reject_connections; });
}
