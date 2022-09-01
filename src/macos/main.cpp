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

#include <iostream>
#include <cstdlib>

static BrokerService service;

void HandleSignal(int signum)
{
  if (signum == SIGTERM)
    service.AsyncShutdown();
}

int main()
{
  if (!service.Init())
    return EXIT_FAILURE;

  // As a launchd daemon, we must set up a SIGTERM handler
  signal(SIGTERM, HandleSignal);

  int retval = EXIT_SUCCESS;
  if (!service.Run())
    retval = EXIT_FAILURE;

  service.Deinit();

  return retval;  // If Run() succeeded, getting here means the main loop terminated, likely due to SIGTERM.
}
