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

#include <CoreFoundation/CoreFoundation.h>
#include <notify_keys.h>

static void InterfaceChangeCallback(CFNotificationCenterRef center,
                                    void*                   observer,
                                    CFStringRef             name,
                                    const void*             object,
                                    CFDictionaryRef         userInfo)
{
  BrokerService* service = reinterpret_cast<BrokerService*>(observer);
  if (service)
    service->RequestRestart();
}

bool BrokerService::Run()
{
  if(shell_thread_.Start([this]() { broker_shell_.Run(); }).IsOk())
  {
    // Set up network change detection
    CFNotificationCenterAddObserver(CFNotificationCenterGetDarwinNotifyCenter(), this, InterfaceChangeCallback,
                                    CFSTR(kNotifySCNetworkChange), nullptr,
                                    CFNotificationSuspensionBehaviorDeliverImmediately);
    CFRunLoopRun();
    return true;
  }
    
  return false;
}

void BrokerService::AsyncShutdown()
{
  CFNotificationCenterRemoveObserver(CFNotificationCenterGetDarwinNotifyCenter(), this, CFSTR(kNotifySCNetworkChange),
                                     nullptr);
  CFRunLoopStop(CFRunLoopGetMain());

  broker_shell_.AsyncShutdown();
  shell_thread_.Join();
}
