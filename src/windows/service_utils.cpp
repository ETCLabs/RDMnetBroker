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

#include <stdio.h>
#include <stddef.h>
#include "service_utils.h"

/// \brief Get the last system error code as a descriptive string.
///
/// The string is copied into the buffer, truncating if necessary.
///
/// \param[out] msg_buf_out Buffer to fill in with the error string.
/// \param[in] buf_size Size of the output buffer.
void GetLastErrorMessage(wchar_t* msg_buf_out, size_t buf_size)
{
  wchar_t* msg_buf;
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
                GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),  // Default language
                (LPWSTR)&msg_buf, 0, nullptr);
  wcsncpy_s(msg_buf_out, buf_size, msg_buf, _TRUNCATE);
  LocalFree(msg_buf);
}

/// \brief Install this executable as a service to the local service control manager database.
///
/// If the function fails to install the service, the resulting error message is printed to stdout.
///
/// \param[in] service_name The name of the service.
/// \param[in] service_display_name The user-facing name that will display in the Services dialog.
/// \param[in] start_type The service start option; one of SERVICE_AUTO_START, SERVICE_BOOT_START,
///                       SERVICE_DEMAND_START, SERVICE_DISABLED, SERVICE_SYSTEM_START.
/// \param[in] dependencies Pointer to a double null-terminated array of null-separated names of
///                         services or load ordering groups that the system must start before this
///                         service.
void InstallService(const wchar_t* service_name, const wchar_t* display_name, DWORD start_type,
                    const wchar_t* dependencies)
{
  constexpr size_t kErrMsgSize = 256;
  wchar_t error_msg[kErrMsgSize];

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
  SC_HANDLE service_handle = CreateService(manager_handle,             // SCManager database
                                           service_name,               // Name of service
                                           display_name,               // Name to display
                                           SERVICE_QUERY_STATUS,       // Desired access
                                           SERVICE_WIN32_OWN_PROCESS,  // Service type
                                           start_type,                 // Service start type
                                           SERVICE_ERROR_NORMAL,       // Error control type
                                           my_file_name,               // Service's binary
                                           nullptr,                    // No load ordering group
                                           nullptr,                    // No tag identifier
                                           dependencies,               // Dependencies
                                           nullptr,                    // Service running account
                                           nullptr                     // Password of the account
  );

  if (service_handle)
  {
    wprintf(L"%s is installed.\n", service_name);
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
/// \param[in] service_name The name of the service to be removed.
void UninstallService(const wchar_t* service_name)
{
  constexpr size_t kErrMsgSize = 256;
  wchar_t error_msg[kErrMsgSize];

  // Open the local default service control manager database
  SC_HANDLE manager_handle = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
  if (!manager_handle)
  {
    GetLastErrorMessage(error_msg, kErrMsgSize);
    wprintf(L"UninstallService: OpenSCManager failed with error: '%s'\n", error_msg);
    return;
  }

  // Open the service with delete, stop, and query status permissions
  SC_HANDLE service_handle = OpenService(manager_handle, service_name, SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE);
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
    wprintf(L"Stopping %s.", service_name);
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
      wprintf(L"\n%s is stopped.\n", service_name);
    }
    else
    {
      wprintf(L"\n%s failed to stop.\n", service_name);
    }
  }

  // Now remove the service by calling DeleteService.
  if (DeleteService(service_handle))
  {
    wprintf(L"%s is removed.\n", service_name);
  }
  else
  {
    GetLastErrorMessage(error_msg, kErrMsgSize);
    wprintf(L"UninstallService: DeleteService failed with error: '%s'\n", error_msg);
  }

  CloseServiceHandle(service_handle);
  CloseServiceHandle(manager_handle);
}
