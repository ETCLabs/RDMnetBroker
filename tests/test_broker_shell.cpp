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

#include "broker_shell.h"

#include "gmock/gmock.h"
#include "broker_os_interface.h"

using testing::_;
using testing::ByMove;
using testing::Return;

class MockBrokerOsInterface : public BrokerOsInterface
{
public:
  MOCK_METHOD(std::string, GetLogFilePath, (), (const override));
  MOCK_METHOD(bool, OpenLogFile, (), (override));
  MOCK_METHOD((std::pair<std::string, std::ifstream>), GetConfFile, (etcpal::Logger & log), (override));
  MOCK_METHOD(etcpal::LogTimestamp, GetLogTimestamp, (), (override));
  MOCK_METHOD(void, HandleLogMessage, (const EtcPalLogStrings& strings), (override));
};

class TestBrokerShell : public testing::Test
{
protected:
  TestBrokerShell()
  {
    ON_CALL(os_interface_, OpenLogFile()).WillByDefault(Return(true));
    // ON_CALL(os_interface_, GetConfFile(_)).WillByDefault(Return(true));
  }

  testing::NiceMock<MockBrokerOsInterface> os_interface_;
  BrokerShell                              shell_{os_interface_};
};

// TODO - if these tests don't work as expected, we might start a real broker and enter an infinite
// loop. This will be caught by the CTest timeout but it's not ideal. RDMNET-137 has been created
// to track making the broker mockable to avoid this situation.

TEST_F(TestBrokerShell, DoesNotStartIfOpenLogFileFails)
{
  EXPECT_CALL(os_interface_, OpenLogFile()).WillOnce(Return(false));
  EXPECT_FALSE(shell_.Run());
}

TEST_F(TestBrokerShell, DoesNotStartIfGetConfFileFails)
{
  EXPECT_CALL(os_interface_, GetConfFile(_)).WillOnce(Return(ByMove(std::make_pair("fake_path", std::ifstream{}))));
  EXPECT_FALSE(shell_.Run());
}
