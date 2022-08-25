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

#include <string>
#include <winsock2.h>
#include <windows.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>
#include "broker_shell.h"
#include "win_broker_os_interface.h"

class BrokerService
{
public:
  // Register the executable for a service with the Service Control Manager (SCM). After you call
  // Run(BrokerService*), the SCM issues a Start command, which results in a call to the OnStart
  // method in the service. This method blocks until the service has stopped.
  static bool RunService(BrokerService* service);

  bool Init() { return broker_shell_.Init(); }
  void Deinit() { broker_shell_.Deinit(); }

  int Debug() { return (broker_shell_.Run(true) ? 0 : 1); }

  void SetServiceStatus(DWORD current_state, DWORD win32_error = NO_ERROR, DWORD service_specific_error = 0);
  void PrintVersion() { broker_shell_.PrintVersion(); }

  BrokerService(const wchar_t* service_name);

private:
  void WriteEventLogEntry(PCWSTR message, WORD type);
  void WriteErrorLogEntry(PCWSTR function_name, DWORD error = GetLastError());

  static void WINAPI ServiceCtrlHandler(DWORD control_code);
  static void WINAPI ServiceMain(DWORD argc, LPWSTR* argv);

  void Start(DWORD argc, PWSTR* argv);
  void Stop();
  void Shutdown();

  static DWORD WINAPI ServiceThread(LPVOID* arg);

  static VOID NETIOAPI_API_ InterfaceChangeCallback(IN PVOID                 CallerContext,
                                                    IN PMIB_IPINTERFACE_ROW  Row,
                                                    IN MIB_NOTIFICATION_TYPE NotificationType);

  static HANDLE InitConfigChangeDetectionHandle();
  static bool   ProcessConfigChanges(HANDLE change_handle);

  static BrokerService* service_;  // The singleton service instance.

  WindowsBrokerOsInterface os_interface_;
  BrokerShell              broker_shell_{os_interface_};

  std::wstring          name_;                    // The name of the service
  SERVICE_STATUS        status_{};                // The status of the service
  SERVICE_STATUS_HANDLE status_handle_{nullptr};  // The service status handle
  HANDLE                service_thread_{nullptr};
};

#endif  // BROKER_SERVICE_H_
