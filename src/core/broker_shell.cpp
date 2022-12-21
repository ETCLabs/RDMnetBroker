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

bool BrokerShell::Init()
{
  if (OpenLogFile())
  {
    if (log_.Startup(os_interface_))
    {
      LoadBrokerConfig();
      log_.SetLogMask(broker_config_.log_mask);
      ready_to_run_ = true;
    }
  }

  return ready_to_run_;
}

void BrokerShell::Deinit()
{
  if (ready_to_run_)
    log_.Shutdown();
}

bool BrokerShell::Run()
{
  if (!ready_to_run_)
    return false;

  if (!rdmnet::Init(log_))
    return false;

  bool startup_broker = true;
  while (true)
  {
    if (startup_broker)
    {
      startup_broker = false;

      if (broker_config_.enable_broker)
      {
        if (etcpal_netint_refresh_interfaces() != kEtcPalErrOk)
          log_.Error("Error refreshing network interfaces - broker may not work correctly.");

        auto res = broker_.Startup(broker_config_.settings, &log_, this);
        if (!res)
        {
          log_.Notice("Broker startup failed (%s), running with broker functionality disabled.", res.ToCString());
          broker_config_.enable_broker = false;
        }
      }
      else
      {
        log_.Info("Running with broker functionality disabled.");
      }
    }

    if (shutdown_requested_)
    {
      break;
    }
    else if (TimeToRestartBroker())
    {
      log_.Info("Restart requested, restarting broker and applying changes...");

      if (broker_config_.enable_broker)
        broker_.Shutdown();

      LoadBrokerConfig();
      ApplySettingsChanges();

      startup_broker = true;
    }

    etcpal_thread_sleep(300);
  }

  if (broker_config_.enable_broker)
    broker_.Shutdown();

  rdmnet::Deinit();
  return true;
}

void BrokerShell::RequestRestart(uint32_t cooldown_ms)
{
  etcpal::MutexGuard guard(lock_);
  LockedRequestRestart(cooldown_ms);
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

void BrokerShell::LoadBrokerConfig()
{
  broker_config_.SetDefaults();  // Start with defaults - settings will be changed as needed.

  auto conf_file_pair = os_interface_.GetConfFile(log_);
  if (!conf_file_pair.second.is_open())
  {
    broker_config_.enable_broker = false;
    if (conf_file_pair.first.empty())
      log_.Notice("Error opening configuration file.");
    else
      log_.Notice("Error opening configuration file located at path \"%s\".", conf_file_pair.first.c_str());
  }

  log_.Info("Reading configuration file at %s...", conf_file_pair.first.c_str());

  auto parse_res = broker_config_.Read(conf_file_pair.second, &log_);

  // kInvalidSetting is treated as non-fatal because it makes sure default values are used in place of invalid ones.
  if ((parse_res != BrokerConfig::ParseResult::kOk) && (parse_res != BrokerConfig::ParseResult::kInvalidSetting))
    broker_config_.enable_broker = false;  // Error was already logged in the Read call above.
}

void BrokerShell::HandleScopeChanged(const std::string& new_scope)
{
  etcpal::MutexGuard guard(lock_);
  new_scope_ = new_scope;
  LockedRequestRestart();
}

void BrokerShell::ApplySettingsChanges()
{
  etcpal::MutexGuard guard(lock_);

  log_.SetLogMask(broker_config_.log_mask);

  if (!new_scope_.empty())
  {
    broker_config_.settings.scope = new_scope_;
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

void BrokerShell::LockedRequestRestart(uint32_t cooldown_ms)
{
  restart_requested_ = true;

  if (cooldown_ms > restart_timer_.GetRemaining())  // Don't cancel out previous cooldown
    restart_timer_.Start(cooldown_ms);
}
