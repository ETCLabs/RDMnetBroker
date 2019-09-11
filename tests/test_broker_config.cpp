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
#include "gtest/gtest.h"

class TestBrokerConfig : public testing::Test
{
protected:
  BrokerConfig config_;
};

TEST_F(TestBrokerConfig, invalid_json_should_fail_without_throwing)
{
  // We're not trying to replicate the (very comprehensive!) testing of our 3rd-party JSON library,
  // just feed our config parser a small set of invalid JSON values to make sure it handles errors
  // from the library correctly.

  // clang-format off
  static const std::vector<std::string> kInvalidJsonStrings =
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
    EXPECT_EQ(config_.ReadFromStream(test_stream), BrokerConfig::ParseResult::kJsonParseErr)
        << "Input tested: " << invalid_input;
  }
}

TEST_F(TestBrokerConfig, invalid_cid_value_should_fail)
{
  // clang-format off
  static const std::vector<std::string> kInvalidCidStrings =
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
    EXPECT_EQ(config_.ReadFromStream(test_stream), BrokerConfig::ParseResult::kInvalidSetting)
        << "Input tested: " << invalid_input;
  }
}
