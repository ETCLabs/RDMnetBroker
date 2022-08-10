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

#ifndef BROKER_SHELL_H_
#define BROKER_SHELL_H_

#include <string>
#include <vector>
#include <array>
#include <atomic>
#include "etcpal/inet.h"
#include "etcpal/cpp/mutex.h"
#include "etcpal/cpp/log.h"
#include "etcpal/cpp/timer.h"
#include "rdmnet/cpp/broker.h"
#include "broker_config.h"
#include "broker_os_interface.h"

// BrokerShell : Platform-neutral wrapper around the Broker library from a generic console
// application. Instantiates and drives the Broker library.

class BrokerShell : public rdmnet::Broker::NotifyHandler
{
public:
  BrokerShell(BrokerOsInterface& os_interface);
  ~BrokerShell();

  bool Run(bool debug_mode = false);

  void RequestRestart(uint32_t cooldown_ms = 0u);
  void AsyncShutdown();

  void PrintVersion();

  etcpal::Logger& log() { return log_; }

private:
  BrokerOsInterface& os_interface_;
  rdmnet::Broker     broker_;
  etcpal::Logger     log_;

  BrokerConfig broker_config_;

  bool ready_to_run_{false};

  // Handle changes at runtime
  mutable etcpal::Mutex lock_;  // These are guarded by this lock
  etcpal::Timer         restart_timer_;
  bool                  restart_requested_{false};
  bool                  shutdown_requested_{false};
  std::string           new_scope_;

  bool OpenLogFile();
  bool LoadBrokerConfig();

  void HandleScopeChanged(const std::string& new_scope) override;
  void PrintWarningMessage();

  void ApplySettingsChanges();

  bool TimeToRestartBroker();

  void LockedRequestRestart(uint32_t cooldown_ms = 0u);
};

#endif  // BROKER_SHELL_H_
