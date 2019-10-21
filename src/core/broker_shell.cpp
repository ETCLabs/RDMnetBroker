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

#include "broker_shell.h"

#include <iostream>
#include <cstring>
#include "etcpal/netint.h"
#include "etcpal/thread.h"
#include "rdmnet/version.h"

void BrokerShell::HandleScopeChanged(const std::string& new_scope)
{
  if (log_)
    log_->Info("Scope change detected, restarting broker and applying changes");

  new_scope_ = new_scope;
  restart_requested_ = true;
}

void BrokerShell::NetworkChanged()
{
  if (log_)
    log_->Info("Network change detected, restarting broker and applying changes");

  restart_requested_ = true;
}

void BrokerShell::AsyncShutdown()
{
  if (log_)
    log_->Info("Shutdown requested, Broker shutting down...");

  shutdown_requested_ = true;
}

void BrokerShell::ApplySettingsChanges(rdmnet::BrokerSettings& settings)
{
  if (!new_scope_.empty())
  {
    settings.scope = new_scope_;
    new_scope_.clear();
  }
}

void BrokerShell::Run(rdmnet::BrokerLog* log, bool /*debug_mode*/)
{
  log_ = log;
  if (log_)
    log_->Startup(ETCPAL_LOG_DEBUG);

  rdmnet::BrokerSettings broker_settings;

  broker_settings.dns.manufacturer = "ETC";
  broker_settings.dns.service_instance_name = "UNIQUE NAME";
  broker_settings.dns.model = "E1.33 Broker Prototype";

  broker_.Startup(broker_settings, this, log_);

  // We want this to run forever if a console
  while (true)
  {
    broker_.Tick();

    if (shutdown_requested_)
    {
      break;
    }
    else if (restart_requested_)
    {
      restart_requested_ = false;

      broker_settings = broker_.GetSettings();
      broker_.Shutdown();

      ApplySettingsChanges(broker_settings);
      broker_.Startup(broker_settings, this, log_);
    }

    etcpal_thread_sleep(300);
  }

  broker_.Shutdown();
  if (log_)
    log_->Shutdown();
}
