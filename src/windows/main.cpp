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

// The Windows entry point for the broker service.

#include <memory>
#include <cstdio>
#include <winsock2.h>
#include <windows.h>
#include "service_config.h"
#include "service_utils.h"
#include "broker_service.h"
#include "broker_version.h"

void PrintVersion()
{
  std::printf("%s version %s\n", BrokerVersion::ProductNameString().c_str(), BrokerVersion::VersionString().c_str());
}

void PrintUsage(const wchar_t* app_name)
{
  std::wprintf(L"Usage: %s [OPTIONAL_ACTION]\n", app_name);
  std::wprintf(L"Note: Only the Windows Service Control Manager should invoke this executable with no options.\n");
  std::wprintf(L"\n");
  std::wprintf(L"Optional actions (only one may be specified at a time):\n");
  std::wprintf(L"  -install  Install the service.\n");
  std::wprintf(L"  -remove   Remove the service.\n");
  std::wprintf(L"  -debug    Run the service executable directly in this console for debugging.\n");
  std::wprintf(L"  -version  Print version information and exit.\n");
}

int wmain(int argc, wchar_t* argv[])
{
  bool debug_mode = false;

  auto service = std::make_unique<BrokerService>(kServiceName);
  if (!service)
  {
    std::wprintf(L"Error: Couldn't instantiate Broker service.\n");
    return 1;
  }

  if (!service->Init())
  {
    std::wprintf(L"Error: Couldn't initialize Broker service.\n");
    return 1;
  }

  bool run_service = true;
  int  retval = 0;
  if (argc > 1)
  {
    if (_wcsicmp(L"-version", argv[1]) == 0)
    {
      service->PrintVersion();
      run_service = false;
    }
    if (_wcsicmp(L"-install", argv[1]) == 0)
    {
      InstallService();
      run_service = false;
    }
    else if (_wcsicmp(L"-remove", argv[1]) == 0)
    {
      UninstallService();
      run_service = false;
    }
    else if (_wcsicmp(L"-debug", argv[1]) == 0)
    {
      debug_mode = true;
    }
    else
    {
      PrintUsage(argv[0]);
      run_service = false;
      retval = 1;
    }
  }

  // Got to here without returning - we will either run or debug.
  if (debug_mode)
  {
    retval = service->Debug();
  }
  else if (run_service)
  {
    if (!BrokerService::RunService(service.get()))
    {
      wchar_t error_msg[256];
      GetLastErrorMessage(error_msg, 256);
      std::wprintf(L"Service failed to run with error: '%s'\n", error_msg);
      retval = 1;
    }
  }

  service->Deinit();

  return retval;
}
