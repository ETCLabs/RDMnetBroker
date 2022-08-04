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

#ifndef BROKER_SERVICE_H_
#define BROKER_SERVICE_H_

#include "broker_shell.h"
#include "mac_broker_os_interface.h"
#include "etcpal/cpp/thread.h"

class BrokerService
{
public:
  bool Run();
  void AsyncShutdown();

  void PrintVersion() { broker_shell_.PrintVersion(); }

  void RequestRestart(uint32_t cooldown_ms = 0u) { broker_shell_.RequestRestart(cooldown_ms); }

private:
  MacBrokerOsInterface os_interface_;
  BrokerShell          broker_shell_{os_interface_};

  etcpal::Thread shell_thread_;
};

#endif  // BROKER_SERVICE_H_
