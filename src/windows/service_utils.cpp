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

#include <stdio.h>
#include <stddef.h>
#include <string>
#include "broker_common.h"
#include "service_utils.h"
#include "service_config.h"

/// \brief Get the last system error code as a descriptive string.
///
/// The string is copied into the buffer, truncating if necessary.
///
/// \param[out] msg_buf_out Buffer to fill in with the error string.
/// \param[in] buf_size Size of the output buffer.
void GetLastErrorMessage(wchar_t* msg_buf_out, size_t buf_size)
{
  if (msg_buf_out)
    GetLastErrorMessage(GetLastError(), msg_buf_out, buf_size);
}

/// \brief Get a descriptive string for the given error code.
///
/// The string is copied into the buffer, truncating if necessary.
///
/// \param[in] code Error code for which to get the message.
/// \param[out] msg_buf_out Buffer to fill in with the error string.
/// \param[in] buf_size Size of the output buffer.
void GetLastErrorMessage(DWORD code, wchar_t* msg_buf_out, size_t buf_size)
{
  if (msg_buf_out)
  {
    wchar_t* msg_buf = nullptr;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
                  code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),  // Default language
                  (LPWSTR)&msg_buf, 0, nullptr);

    if (msg_buf)
    {
      wcsncpy_s(msg_buf_out, buf_size, msg_buf, _TRUNCATE);
      LocalFree(msg_buf);
    }
  }
}

/// \brief Install this executable as a service to the local service control manager database.
///
/// If the function fails to install the service, the resulting error message is printed to stdout.
///
/// Uses the service parameters from the generated header service_config.h
void InstallService()
{
  constexpr size_t kErrMsgSize = 256;
  wchar_t          error_msg[kErrMsgSize];

  wchar_t my_file_name[MAX_PATH];
  if (GetModuleFileName(nullptr, my_file_name, MAX_PATH) == 0)
  {
    GetLastErrorMessage(error_msg, kErrMsgSize);
    wprintf(L"InstallService: GetModuleFileName failed with error: '%s'\n", error_msg);
    return;
  }

  // Open the local default service control manager database
  SC_HANDLE manager_handle = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE);
  if (!manager_handle)
  {
    GetLastErrorMessage(error_msg, kErrMsgSize);
    wprintf(L"InstallService: OpenSCManager failed with error: '%s'\n", error_msg);
    return;
  }

  // Install the service into the Service Control Manager
  SC_HANDLE service_handle = CreateService(manager_handle,                                // SCManager database
                                           kServiceName,                                  // Name of service
                                           kServiceDisplayName,                           // Name to display
                                           SERVICE_QUERY_STATUS | SERVICE_CHANGE_CONFIG,  // Desired access
                                           SERVICE_WIN32_OWN_PROCESS,                     // Service type
                                           kServiceStartType,                             // Service start type
                                           kServiceErrorControl,                          // Error control type
                                           my_file_name,                                  // Service's binary
                                           nullptr,                                       // No load ordering group
                                           nullptr,                                       // No tag identifier
                                           nullptr,                                       // Dependencies
                                           nullptr,                                       // Service running account
                                           nullptr                                        // Password of the account
  );

  if (service_handle)
  {
    // Edit the service config

    // Set the service description
    // Sigh... it's too much to ask for these APIs to be const-correct, isn't it?
    std::wstring        non_const_description = kServiceDescription;
    SERVICE_DESCRIPTION service_desc;
    service_desc.lpDescription = non_const_description.data();
    ChangeServiceConfig2(service_handle, SERVICE_CONFIG_DESCRIPTION, &service_desc);

    // Set the failure action - restart after 5 seconds
    SERVICE_FAILURE_ACTIONS fail_actions;
    fail_actions.dwResetPeriod = INFINITE;
    fail_actions.lpRebootMsg = NULL;
    fail_actions.lpCommand = NULL;
    fail_actions.cActions = static_cast<DWORD>(std::size(kServiceFailureActions));
    fail_actions.lpsaActions = kServiceFailureActions;

    ChangeServiceConfig2(service_handle, SERVICE_CONFIG_FAILURE_ACTIONS, &fail_actions);

    wprintf(L"%s is installed.\n", kServiceName);
    CloseServiceHandle(service_handle);
  }
  else
  {
    GetLastErrorMessage(error_msg, kErrMsgSize);
    wprintf(L"InstallService: CreateService failed with error: '%s'\n", error_msg);
  }

  CloseServiceHandle(manager_handle);
}

/// \brief Stop and remove a service from the local service control manager database.
///
/// If the function fails to uninstall the service, the resulting error message is printed to
/// stdout.
///
/// Uses the service parameters from the generated header service_config.h
void UninstallService()
{
  constexpr size_t kErrMsgSize = 256;
  wchar_t          error_msg[kErrMsgSize];

  // Open the local default service control manager database
  SC_HANDLE manager_handle = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
  if (!manager_handle)
  {
    GetLastErrorMessage(error_msg, kErrMsgSize);
    wprintf(L"UninstallService: OpenSCManager failed with error: '%s'\n", error_msg);
    return;
  }

  // Open the service with delete, stop, and query status permissions
  SC_HANDLE service_handle = OpenService(manager_handle, kServiceName, SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);
  if (!service_handle)
  {
    GetLastErrorMessage(error_msg, kErrMsgSize);
    wprintf(L"OpenService failed with error: '%s'\n", error_msg);
    CloseServiceHandle(manager_handle);
    return;
  }

  // Try to stop the service
  SERVICE_STATUS status = {};
  if (ControlService(service_handle, SERVICE_CONTROL_STOP, &status))
  {
    wprintf(L"Stopping %s.", kServiceName);
    Sleep(1000);

    while (QueryServiceStatus(service_handle, &status))
    {
      if (status.dwCurrentState == SERVICE_STOP_PENDING)
      {
        wprintf(L".");
        Sleep(1000);
      }
      else
        break;
    }

    if (status.dwCurrentState == SERVICE_STOPPED)
    {
      wprintf(L"\n%s is stopped.\n", kServiceName);
    }
    else
    {
      wprintf(L"\n%s failed to stop.\n", kServiceName);
    }
  }

  // Now remove the service by calling DeleteService.
  if (DeleteService(service_handle))
  {
    wprintf(L"%s is removed.\n", kServiceName);
  }
  else
  {
    GetLastErrorMessage(error_msg, kErrMsgSize);
    wprintf(L"UninstallService: DeleteService failed with error: '%s'\n", error_msg);
  }

  CloseServiceHandle(service_handle);
  CloseServiceHandle(manager_handle);
}
