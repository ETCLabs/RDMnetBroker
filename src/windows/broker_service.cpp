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

#include "broker_service.h"
#include <cassert>
#include <strsafe.h>

BrokerService* BrokerService::service_{nullptr};

// The system will deliver this callback when an IPv4 or IPv6 network adapter changes state. This
// event is passed along to the BrokerShell instance, which restarts the broker.
VOID NETIOAPI_API_ BrokerService::InterfaceChangeCallback(IN PVOID                 CallerContext,
                                                          IN PMIB_IPINTERFACE_ROW  Row,
                                                          IN MIB_NOTIFICATION_TYPE NotificationType)
{
  (void)CallerContext;
  (void)Row;
  (void)NotificationType;

  if (service_)
  {
    service_->broker_shell_.RequestRestart();
  }
}

DWORD WINAPI BrokerService::ServiceThread(LPVOID* /*arg*/)
{
  DWORD result = 1;
  if (service_)
  {
    // Register with Windows for network change detection
    HANDLE change_notif_handle = nullptr;
    NotifyIpInterfaceChange(AF_UNSPEC, InterfaceChangeCallback, nullptr, FALSE, &change_notif_handle);

    if (service_->broker_shell_.Run())
      result = 0;

    // Cancel network change detection
    CancelMibChangeNotify2(change_notif_handle);

    if (result != 0)
    {
      service_->SetServiceStatus(SERVICE_STOPPED, ERROR_SERVICE_SPECIFIC_ERROR, result);
    }
  }
  return result;
}

bool BrokerService::RunService(BrokerService* service)
{
  service_ = service;

  SERVICE_TABLE_ENTRY serviceTable[] = {{const_cast<wchar_t*>(service->name_.c_str()), ServiceMain}, {NULL, NULL}};

  // Connects the main thread of a service process to the service control manager, which causes the
  // thread to be the service control dispatcher thread for the calling process. This call returns
  // when the service has stopped. The process should simply terminate when the call returns.
  return StartServiceCtrlDispatcher(serviceTable);
}

void WINAPI BrokerService::ServiceMain(DWORD argc, PWSTR* argv)
{
  assert(service_ != nullptr);

  // Register the handler function for the service
  service_->status_handle_ = RegisterServiceCtrlHandler(service_->name_.c_str(), ServiceCtrlHandler);
  if (!service_->status_handle_)
  {
    throw GetLastError();
  }

  service_->Start(argc, argv);
}

//   control_code - the control code. This parameter can be one of the
//   following values:
//
//     SERVICE_CONTROL_CONTINUE
//     SERVICE_CONTROL_INTERROGATE
//     SERVICE_CONTROL_NETBINDADD
//     SERVICE_CONTROL_NETBINDDISABLE
//     SERVICE_CONTROL_NETBINDREMOVE
//     SERVICE_CONTROL_PARAMCHANGE
//     SERVICE_CONTROL_PAUSE
//     SERVICE_CONTROL_SHUTDOWN
//     SERVICE_CONTROL_STOP
//
//   This parameter can also be a user-defined control code ranges from 128
//   to 255.
//
void WINAPI BrokerService::ServiceCtrlHandler(DWORD control_code)
{
  switch (control_code)
  {
    case SERVICE_CONTROL_STOP:
      service_->Stop();
      break;
    case SERVICE_CONTROL_SHUTDOWN:
      service_->Shutdown();
      break;
    default:
      break;
  }
}

BrokerService::BrokerService(const wchar_t* service_name) : name_(service_name)
{
  status_.dwServiceType = SERVICE_WIN32_OWN_PROCESS;  // The service runs in its own process.
  status_.dwCurrentState = SERVICE_START_PENDING;     // The service is starting.

  // The accepted commands of the service.
  status_.dwControlsAccepted = (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);

  status_.dwWin32ExitCode = NO_ERROR;
  status_.dwServiceSpecificExitCode = 0;
  status_.dwCheckPoint = 0;
  status_.dwWaitHint = 0;
}

void BrokerService::Start(DWORD /*dwArgc*/, PWSTR* /*pszArgv*/)
{
  try
  {
    // Tell SCM that the service is starting.
    SetServiceStatus(SERVICE_START_PENDING);

    service_thread_ = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)ServiceThread, this, 0, nullptr);

    // Tell SCM that the service is started.
    SetServiceStatus(SERVICE_RUNNING);
  }
  catch (DWORD error)
  {
    // Log the error.
    WriteErrorLogEntry(L"Service Start", error);

    // Set the service status to be stopped.
    SetServiceStatus(SERVICE_STOPPED, error);
  }
  catch (...)
  {
    // Log the error.
    WriteEventLogEntry(L"Service failed to start.", EVENTLOG_ERROR_TYPE);

    // Set the service status to be stopped.
    SetServiceStatus(SERVICE_STOPPED);
  }
}

void BrokerService::Stop()
{
  DWORD dwOriginalState = status_.dwCurrentState;
  try
  {
    // Tell SCM that the service is stopping.
    SetServiceStatus(SERVICE_STOP_PENDING);

    // Perform service-specific stop operations.
    broker_shell_.AsyncShutdown();
    WaitForSingleObject(service_thread_, INFINITE);

    // Tell SCM that the service is stopped.
    SetServiceStatus(SERVICE_STOPPED);
  }
  catch (DWORD dwError)
  {
    // Log the error.
    WriteErrorLogEntry(L"Service Stop", dwError);

    // Set the orginal service status.
    SetServiceStatus(dwOriginalState);
  }
  catch (...)
  {
    // Log the error.
    WriteEventLogEntry(L"Service failed to stop.", EVENTLOG_ERROR_TYPE);

    // Set the orginal service status.
    SetServiceStatus(dwOriginalState);
  }
}

void BrokerService::Shutdown()
{
  try
  {
    // Perform service-specific stop operations.
    broker_shell_.AsyncShutdown();
    WaitForSingleObject(service_thread_, INFINITE);

    // Tell SCM that the service is stopped.
    SetServiceStatus(SERVICE_STOPPED);
  }
  catch (DWORD dwError)
  {
    // Log the error.
    WriteErrorLogEntry(L"Service Shutdown", dwError);
  }
  catch (...)
  {
    // Log the error.
    WriteEventLogEntry(L"Service failed to shut down.", EVENTLOG_ERROR_TYPE);
  }
}

void BrokerService::SetServiceStatus(DWORD current_state, DWORD win32_error, DWORD service_specific_error)
{
  static DWORD check_point = 1;

  // Fill in the SERVICE_STATUS structure of the service.

  status_.dwCurrentState = current_state;
  status_.dwWin32ExitCode = win32_error;
  status_.dwServiceSpecificExitCode = service_specific_error;
  status_.dwWaitHint = 0;

  status_.dwCheckPoint = ((current_state == SERVICE_RUNNING) || (current_state == SERVICE_STOPPED)) ? 0 : check_point++;

  // Report the status of the service to the SCM.
  ::SetServiceStatus(status_handle_, &status_);
}

void BrokerService::WriteEventLogEntry(PCWSTR message, WORD type)
{
  HANDLE event_src_handle = RegisterEventSource(NULL, name_.c_str());
  if (event_src_handle)
  {
    LPCWSTR string_arr[2] = {nullptr, nullptr};
    string_arr[0] = name_.c_str();
    string_arr[1] = message;

    ReportEvent(event_src_handle,  // Event log handle
                type,              // Event type
                0,                 // Event category
                0,                 // Event identifier
                NULL,              // No security identifier
                2,                 // Size of lpszStrings array
                0,                 // No binary data
                string_arr,        // Array of strings
                NULL               // No binary data
    );

    DeregisterEventSource(event_src_handle);
  }
}

void BrokerService::WriteErrorLogEntry(PCWSTR function_name, DWORD error)
{
  constexpr size_t kMsgSize = 260;
  wchar_t          message[kMsgSize];
  StringCchPrintf(message, kMsgSize, L"%s failed with error 0x%08lx", function_name, error);
  WriteEventLogEntry(message, EVENTLOG_ERROR_TYPE);
}
