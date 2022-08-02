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

#include "broker_shell.h"

#include <iostream>
#include <cstring>
#include "etcpal/netint.h"
#include "etcpal/thread.h"
#include "rdmnet/cpp/common.h"
#include "broker_version.h"

// The interval to wait before restarting (in case we get blasted with tons of notifications at once)
static constexpr uint32_t kRestartTimerIntervalMs = 2000u;

bool BrokerShell::Run(bool /*debug_mode*/)
{
  if (!OpenLogFile())
    return false;

  if (!log_.Startup(os_interface_))
    return false;

  if (!LoadBrokerConfig())
  {
    log_.Shutdown();
    return false;
  }

  log_.SetLogMask(broker_config_.log_mask);

  if (!rdmnet::Init(log_))
  {
    log_.Shutdown();
    return false;
  }

  if (!broker_.Startup(broker_config_.settings, &log_, this))
  {
    rdmnet::Deinit();
    log_.Shutdown();
    return false;
  }

  while (true)
  {
    if (shutdown_requested_)
    {
      break;
    }
    else if (TimeToRestartBroker())
    {
      log_.Info("Restart requested, restarting broker and applying changes");

      auto broker_settings = broker_.settings();
      broker_.Shutdown();

      ApplySettingsChanges(broker_settings);
      broker_.Startup(broker_settings, &log_, this);
    }

    etcpal_thread_sleep(300);
  }

  broker_.Shutdown();
  rdmnet::Deinit();
  log_.Shutdown();
  return true;
}

void BrokerShell::RequestRestart()
{
  etcpal::MutexGuard guard(lock_);
  LockedRequestRestart();
}

void BrokerShell::AsyncShutdown()
{
  log_.Info("Shutdown requested, Broker shutting down...");
  shutdown_requested_ = true;
}

void BrokerShell::PrintVersion()
{
  std::cout << BrokerVersion::ProductNameString() << '\n';
  std::cout << "Version " << BrokerVersion::VersionString() << ", built " << BrokerVersion::BuildDateString() << "\n\n";
  std::cout << BrokerVersion::CopyrightString() << '\n';
  std::cout << "License: Apache License v2.0 <http://www.apache.org/licenses/LICENSE-2.0>\n";
  std::cout << "Unless required by applicable law or agreed to in writing, this software is\n";
  std::cout << "provided \"AS IS\", WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express\n";
  std::cout << "or implied.\n";
}

bool BrokerShell::OpenLogFile()
{
  if (!os_interface_.OpenLogFile())
  {
    log_.Critical("FATAL: Error opening log file for writing at path \"%s\".", os_interface_.GetLogFilePath().c_str());
    return false;
  }
  return true;
}

bool BrokerShell::LoadBrokerConfig()
{
  auto conf_file_pair = os_interface_.GetConfFile(log_);
  if (!conf_file_pair.second.is_open())
  {
    if (conf_file_pair.first.empty())
    {
      log_.Notice("Error opening configuration file. Proceeding with default settings...");
    }
    else
    {
      log_.Notice("Error opening configuration file located at path \"%s\". Proceeding with default settings...",
                  conf_file_pair.first.c_str());
    }
    broker_config_.SetDefaults();
  }

  log_.Info("Reading configuration file at %s...", conf_file_pair.first.c_str());

  auto parse_res = broker_config_.Read(conf_file_pair.second, &log_);
  if (parse_res != BrokerConfig::ParseResult::kOk)
  {
    log_.Critical("FATAL: Error while reading configuration file.");
    return false;
  }
  return true;
}

void BrokerShell::HandleScopeChanged(const std::string& new_scope)
{
  etcpal::MutexGuard guard(lock_);
  new_scope_ = new_scope;
  LockedRequestRestart();
}

void BrokerShell::ApplySettingsChanges(rdmnet::Broker::Settings& settings)
{
  etcpal::MutexGuard guard(lock_);

  if (!new_scope_.empty())
  {
    settings.scope = new_scope_;
    new_scope_.clear();
  }
}

bool BrokerShell::TimeToRestartBroker()
{
  etcpal::MutexGuard guard(lock_);

  // The timer is used to prevent "restart spamming" if tons of restarts are requested at once.
  if (restart_requested_ && restart_timer_.IsExpired())
  {
    restart_requested_ = false;
    return true;
  }

  return false;
}

void BrokerShell::LockedRequestRestart()
{
  restart_requested_ = true;
  restart_timer_.Start(kRestartTimerIntervalMs);
}
