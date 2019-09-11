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

// The Windows entry point for the broker service.

#include <memory>
#include <cstdio>
#include <windows.h>
#include "service_utils.h"
#include "broker_service.h"

constexpr wchar_t kServiceName[] = L"ETC RDMnet Broker";         // Internal name of the service
constexpr wchar_t kServiceDisplayName[] = L"ETC RDMnet Broker";  // Displayed name of the service
constexpr int kServiceStartType = SERVICE_DEMAND_START;          // Service start options.
constexpr wchar_t kServiceDependencies[] = L"";                  // List of service dependencies - "dep1\0dep2\0\0"

void PrintUsage(const wchar_t* app_name)
{
  wprintf(L"Usage: %s [OPTIONAL ACTION]\n", app_name);
  wprintf(L"Note: Only the Windows Service Control Manager should invoke this executable with no options.\n");
  wprintf(L"\n");
  wprintf(L"Optional actions (only one may be specified at a time):\n");
  wprintf(L"  -install  Install the service.\n");
  wprintf(L"  -remove   Remove the service.\n");
  wprintf(L"  -debug    Run the service executable directly in this console for debugging.\n");
}

int wmain(int argc, wchar_t* argv[])
{
  if (argc > 1)
  {
    if (_wcsicmp(L"-install", argv[1]) == 0)
    {
      InstallService(kServiceName, kServiceDisplayName, kServiceStartType, kServiceDependencies);
    }
    else if (_wcsicmp(L"-remove", argv[1]) == 0)
    {
      UninstallService(kServiceName);
    }
    else
    {
      PrintUsage(argv[0]);
      return 1;
    }
  }
  else
  {
    auto service = std::make_unique<BrokerService>(kServiceName);
    if (service)
    {
      if (!BrokerService::Run(service.get()))
      {
        wchar_t error_msg[256];
        GetLastErrorMessage(error_msg, 256);
        wprintf(L"Service failed to run with error: '%s'\n", error_msg);
      }
    }
    else
    {
      wprintf(L"Error: Couldn't instantiate Broker service.\n");
    }
  }

  return 0;
}
