#!/bin/bash

set -e

if /bin/launchctl list "com.etcconnect.pkg.RDMnetBroker" &> /dev/null; then
  /bin/launchctl unload "/Library/LaunchDaemons/com.etcconnect.pkg.RDMnetBroker.plist"
fi

/bin/launchctl load "/Library/LaunchDaemons/com.etcconnect.pkg.RDMnetBroker.plist"
