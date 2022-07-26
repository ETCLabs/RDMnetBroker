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

// The MacOS entry point for the broker service.

#include <signal.h>
#include "broker_service.h"

int main()
{
  // As a launchd daemon, we must set up a SIGTERM handler
  sigset_t term_signal;
  sigemptyset(&term_signal);
  sigaddset(&term_signal, SIGTERM);
  sigprocmask(SIG_BLOCK, &term_signal, nullptr);

  BrokerService service;
  if (!service.Init())
    return 1;

  // Now wait for SIGTERM, then shut down
  int l_signal;
  sigwait(&term_signal, &l_signal);

  service.Deinit();

  return 0;
}
