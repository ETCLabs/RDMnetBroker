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

void BrokerShell::ScopeChanged(const std::string& new_scope)
{
  if (log_)
    log_->Log(ETCPAL_LOG_INFO, "Scope change detected, restarting broker and applying changes");

  new_scope_ = new_scope;
  restart_requested_ = true;
}

void BrokerShell::NetworkChanged()
{
  if (log_)
    log_->Log(ETCPAL_LOG_INFO, "Network change detected, restarting broker and applying changes");

  restart_requested_ = true;
}

void BrokerShell::AsyncShutdown()
{
  if (log_)
    log_->Log(ETCPAL_LOG_INFO, "Shutdown requested, Broker shutting down...");

  shutdown_requested_ = true;
}

void BrokerShell::ApplySettingsChanges(rdmnet::BrokerSettings& settings, std::vector<EtcPalIpAddr>& new_addrs)
{
  new_addrs = GetInterfacesToListen();

  if (!new_scope_.empty())
  {
    settings.disc_attributes.scope = new_scope_;
    new_scope_.clear();
  }
}

std::vector<EtcPalIpAddr> BrokerShell::GetInterfacesToListen()
{
  if (!initial_data_.macs.empty())
  {
    return ConvertMacsToInterfaces(initial_data_.macs);
  }
  else if (!initial_data_.ifaces.empty())
  {
    return initial_data_.ifaces;
  }
  else
  {
    return std::vector<EtcPalIpAddr>();
  }
}

std::vector<EtcPalIpAddr> BrokerShell::ConvertMacsToInterfaces(const std::vector<MacAddress>& macs)
{
  std::vector<EtcPalIpAddr> to_return;

  size_t num_netints = etcpal_netint_get_num_interfaces();
  for (const auto& mac : macs)
  {
    const EtcPalNetintInfo* netint_list = etcpal_netint_get_interfaces();
    for (const EtcPalNetintInfo* netint = netint_list; netint < netint_list + num_netints; ++netint)
    {
      if (0 == memcmp(netint->mac, mac.data(), ETCPAL_NETINTINFO_MAC_LEN))
      {
        to_return.push_back(netint->addr);
        break;
      }
    }
  }

  return to_return;
}

void BrokerShell::Run(rdmnet::BrokerLog* log, rdmnet::BrokerSocketManager* sock_mgr)
{
  log_ = log;
  log_->Startup(initial_data_.log_mask);

  rdmnet::BrokerSettings broker_settings(0x6574);
  broker_settings.disc_attributes.scope = initial_data_.scope;

  std::vector<EtcPalIpAddr> ifaces = GetInterfacesToListen();

  etcpal_generate_v4_uuid(&broker_settings.cid);

  broker_settings.disc_attributes.dns_manufacturer = "ETC";
  broker_settings.disc_attributes.dns_service_instance_name = "UNIQUE NAME";
  broker_settings.disc_attributes.dns_model = "E1.33 Broker Prototype";

  rdmnet::Broker broker(log, sock_mgr, this);
  broker.Startup(broker_settings, initial_data_.port, ifaces);

  // We want this to run forever if a console
  while (true)
  {
    broker.Tick();

    if (shutdown_requested_)
    {
      break;
    }
    else if (restart_requested_)
    {
      restart_requested_ = false;

      broker.GetSettings(broker_settings);
      broker.Shutdown();

      ApplySettingsChanges(broker_settings, ifaces);
      broker.Startup(broker_settings, initial_data_.port, ifaces);
    }

    etcpal_thread_sleep(300);
  }

  broker.Shutdown();
  log_->Shutdown();
}

void BrokerShell::PrintVersion()
{
  std::cout << "ETC Prototype RDMnet Broker\n";
  std::cout << "Version " << RDMNET_VERSION_STRING << "\n\n";
  std::cout << RDMNET_VERSION_COPYRIGHT << "\n";
  std::cout << "License: Apache License v2.0 <http://www.apache.org/licenses/LICENSE-2.0>\n";
  std::cout << "Unless required by applicable law or agreed to in writing, this software is\n";
  std::cout << "provided \"AS IS\", WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express\n";
  std::cout << "or implied.\n";
}
