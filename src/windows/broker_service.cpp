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
#include "broker_common.h"
#include <iostream>
#include <strsafe.h>
#include <system_error>
#include <sstream>

// The interval to wait before restarting (in case we get blasted with tons of notifications at once)
static constexpr uint32_t kNetworkChangeCooldownMs = 5000u;

BrokerService* BrokerService::service_{nullptr};

auto assert_log_fn = [](const char* msg) { std::cout << msg << "\n"; };

bool BrokerService::InitAddrChangeDetection(PHANDLE handle, LPOVERLAPPED overlap)
{
  if (!BROKER_ASSERT_VERIFY(overlap, assert_log_fn) || !BROKER_ASSERT_VERIFY(service_, assert_log_fn))
    return false;

  overlap->hEvent = WSACreateEvent();

  if (overlap->hEvent != WSA_INVALID_EVENT)
    return GetNextAddrChange(handle, overlap);

  service_->broker_shell_.log().Error("ERROR: Failed to set up address table change detection (%s).",
                                      std::system_category().message(GetLastError()).c_str());
  return false;
}

void BrokerService::DeinitAddrChangeDetection(LPOVERLAPPED overlap)
{
  if (!BROKER_ASSERT_VERIFY(overlap, assert_log_fn))
    return;

  CancelIPChangeNotify(overlap);

  if (overlap->hEvent != INVALID_HANDLE_VALUE)
    WSACloseEvent(overlap->hEvent);
}

bool BrokerService::GetNextAddrChange(PHANDLE handle, LPOVERLAPPED overlap)
{
  if (!BROKER_ASSERT_VERIFY(service_, assert_log_fn))
    return false;

  if ((NotifyAddrChange(handle, overlap) != NO_ERROR) && (WSAGetLastError() != WSA_IO_PENDING))
  {
    service_->broker_shell_.log().Error("ERROR: Failed to set up the next address table change notification (%s).",
                                        std::system_category().message(GetLastError()).c_str());
    return false;
  }

  return true;
}

etcpal::Expected<HANDLE> BrokerService::InitConfigChangeDetectionHandle()
{
  if (!BROKER_ASSERT_VERIFY(service_, assert_log_fn))
    return kEtcPalErrSys;

  std::wstring prefix(L"\\\\?\\");  // FindFirstChangeNotification requires this for wide paths
  std::wstring path = prefix + service_->os_interface_.GetConfigPath();
  return FindFirstChangeNotification(path.c_str(), false, FILE_NOTIFY_CHANGE_LAST_WRITE);
}

bool BrokerService::ProcessAddrChanges(PHANDLE handle, LPOVERLAPPED overlap)
{
  if (!BROKER_ASSERT_VERIFY(overlap, assert_log_fn) || !BROKER_ASSERT_VERIFY(service_, assert_log_fn))
    return false;

  static constexpr DWORD kWaitMs = 200u;  // Keep this short for quick shutdown
  DWORD                  status = WaitForSingleObject(overlap->hEvent, kWaitMs);
  switch (status)
  {
    case WAIT_OBJECT_0:  // The address table has changed
      service_->broker_shell_.log().Info("The address table has changed - requesting broker restart.");
      service_->broker_shell_.RequestRestart(kNetworkChangeCooldownMs);
      ResetEvent(overlap->hEvent);
      return GetNextAddrChange(handle, overlap);
    case WAIT_TIMEOUT:  // The address table didn't change, do nothing
      break;
    default:  // WaitForSingleObject error
      service_->broker_shell_.log().Warning(
          "WARNING: Failed to wait for the next address table change notification (GetLastError: '%s', status: %d).",
          std::system_category().message(GetLastError()).c_str(), status);
      return false;
  }

  return true;
}

bool BrokerService::ProcessConfigChanges(HANDLE change_handle)
{
  if (!BROKER_ASSERT_VERIFY(service_, assert_log_fn))
    return false;

  static constexpr DWORD kWaitMs = 200u;  // Keep this short for quick shutdown
  if ((change_handle == INVALID_HANDLE_VALUE) || (change_handle == nullptr))
  {
    service_->broker_shell_.log().Warning("WARNING: Failed to initialize broker config change notification (%s).",
                                          std::system_category().message(GetLastError()).c_str());
    return false;
  }
  else
  {
    DWORD status = WaitForSingleObject(change_handle, kWaitMs);
    switch (status)
    {
      case WAIT_OBJECT_0:  // The config has changed
        service_->broker_shell_.log().Info("The broker configuration file has changed - requesting broker restart.");
        service_->broker_shell_.RequestRestart();
        if (!FindNextChangeNotification(change_handle))
        {
          service_->broker_shell_.log().Warning(
              "WARNING: Failed to set up the next broker config change notification (%s).",
              std::system_category().message(GetLastError()).c_str());
          return false;
        }
        break;
      case WAIT_TIMEOUT:  // The config didn't change, do nothing
        break;
      default:  // WaitForSingleObject error
        service_->broker_shell_.log().Warning(
            "WARNING: Failed to wait for the next broker config change notification (%s).",
            std::system_category().message(GetLastError()).c_str());
        return false;
    }
  }

  return true;
}

DWORD WINAPI BrokerService::ServiceThread(LPVOID* /*arg*/)
{
  if (!BROKER_ASSERT_VERIFY(service_, assert_log_fn))
    return 1;

  DWORD result = 1;

  // Set up network change detection
  bool           stop_addr_change_detection = false;
  etcpal::Thread addr_change_detection_thread([&stop_addr_change_detection]() {
    HANDLE     handle = INVALID_HANDLE_VALUE;
    OVERLAPPED overlap;
    if (InitAddrChangeDetection(&handle, &overlap))
    {
      while (!stop_addr_change_detection && ProcessAddrChanges(&handle, &overlap))
        ;

      DeinitAddrChangeDetection(&overlap);
    }
  });

  // Also set up config change detection
  bool           stop_config_change_detection = false;
  etcpal::Thread config_change_detection_thread([&stop_config_change_detection]() {
    auto change_handle = InitConfigChangeDetectionHandle();
    while (change_handle && !stop_config_change_detection && ProcessConfigChanges(*change_handle))
      ;
  });

  if (service_->broker_shell_.Run())
    result = 0;

  // Stop config change detection
  stop_config_change_detection = true;
  config_change_detection_thread.Join();

  // Cancel network change detection
  stop_addr_change_detection = true;
  addr_change_detection_thread.Join();

  if (result != 0)
  {
    service_->SetServiceStatus(SERVICE_STOPPED, ERROR_SERVICE_SPECIFIC_ERROR, result);
  }

  return result;
}

bool BrokerService::RunService(BrokerService* service)
{
  if (!BROKER_ASSERT_VERIFY(service, assert_log_fn))
    return false;

  service_ = service;

  SERVICE_TABLE_ENTRY serviceTable[] = {{const_cast<wchar_t*>(service->name_.c_str()), ServiceMain}, {NULL, NULL}};

  // Connects the main thread of a service process to the service control manager, which causes the
  // thread to be the service control dispatcher thread for the calling process. This call returns
  // when the service has stopped. The process should simply terminate when the call returns.
  return StartServiceCtrlDispatcher(serviceTable);
}

void WINAPI BrokerService::ServiceMain(DWORD argc, PWSTR* argv)
{
  if (!BROKER_ASSERT_VERIFY(service_, assert_log_fn))
    return;

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
  if (!BROKER_ASSERT_VERIFY(service_, assert_log_fn))
    return;

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

BrokerService::BrokerService(const wchar_t* service_name)
{
  if (BROKER_ASSERT_VERIFY(service_name, assert_log_fn))
    name_ = std::wstring(service_name);

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
