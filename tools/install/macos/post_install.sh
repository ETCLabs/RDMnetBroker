#!/bin/bash

set -e

CONFIG_BACKUP=/usr/local/etc/RDMnetBroker/broker.conf.backup
CONFIG_TO_RESTORE=/usr/local/etc/RDMnetBroker/broker.conf
if test -f "$CONFIG_BACKUP"; then
  cp -p $CONFIG_BACKUP $CONFIG_TO_RESTORE
  rm $CONFIG_BACKUP
fi

/bin/launchctl load "/Library/LaunchDaemons/com.etcconnect.pkg.RDMnetBroker.plist"
/bin/launchctl load "/Library/LaunchDaemons/com.etcconnect.pkg.RDMnetBrokerRestarter.plist"
