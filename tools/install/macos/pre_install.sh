#!/bin/bash

set -e

if /bin/launchctl list "com.etcconnect.pkg.RDMnetBrokerRestarter" &> /dev/null; then
  /bin/launchctl unload "/Library/LaunchDaemons/com.etcconnect.pkg.RDMnetBrokerRestarter.plist"
fi
if /bin/launchctl list "com.etcconnect.pkg.RDMnetBroker" &> /dev/null; then
  /bin/launchctl unload "/Library/LaunchDaemons/com.etcconnect.pkg.RDMnetBroker.plist"
fi

EXISTING_CONFIG=/usr/local/etc/RDMnetBroker/broker.conf
CONFIG_BACKUP=/usr/local/etc/RDMnetBroker/broker.conf.backup
if test -f "$EXISTING_CONFIG"; then
  cp -p $EXISTING_CONFIG $CONFIG_BACKUP
fi
