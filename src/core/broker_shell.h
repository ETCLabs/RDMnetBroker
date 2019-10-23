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

#ifndef BROKER_SHELL_H_
#define BROKER_SHELL_H_

#include <string>
#include <vector>
#include <array>
#include <atomic>
#include "etcpal/inet.h"
#include "etcpal/log.h"
#include "rdmnet/broker.h"
#include "broker_config.h"

// BrokerShell : Platform-neutral wrapper around the Broker library from a generic console
// application. Instantiates and drives the Broker library.

class BrokerShell : public rdmnet::BrokerNotify
{
public:
  bool Run(rdmnet::BrokerLog& log, bool debug_mode = false);

  void NetworkChanged();
  void AsyncShutdown();

protected:
  BrokerConfig broker_config_;

  virtual bool LoadBrokerConfig(rdmnet::BrokerLog& log) = 0;

private:
  rdmnet::Broker broker_;
  rdmnet::BrokerLog* log_{nullptr};

  // Handle changes at runtime
  std::atomic<bool> restart_requested_{false};
  bool shutdown_requested_{false};
  std::string new_scope_;

  void HandleScopeChanged(const std::string& new_scope) override;
  void PrintWarningMessage();

  void ApplySettingsChanges(rdmnet::BrokerSettings& settings);
};

#endif  // BROKER_SHELL_H_
