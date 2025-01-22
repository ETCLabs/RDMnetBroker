"""Script to build, sign and notarize the macOS RDMnetBroker binary package."""
import datetime
import os
import re
import subprocess
import sys
import time

PACKAGE_BUNDLE_ID = "com.etcconnect.pkg.RDMnetBroker"

MACOS_APPLICATION_SIGNING_IDENTITY = os.getenv(
    "RDMNETBRKR_MACOS_APPLICATION_SIGNING_IDENTITY",
    "Developer ID Application: Electronic Theatre Controls, Inc. (8AVSFD7ZED)",
)
MACOS_INSTALLER_SIGNING_IDENTITY = os.getenv(
    "RDMNETBRKR_MACOS_INSTALLER_SIGNING_IDENTITY",
    "Developer ID Installer: Electronic Theatre Controls, Inc. (8AVSFD7ZED)",
)
KEYCHAIN_PASSWORD = os.getenv("RDMNETBRKR_MACOS_KEYCHAIN_PASSWORD")

DEVEL_ID_USERNAME = os.getenv("RDMNETBRKR_APPLE_DEVELOPER_ID_USER")
DEVEL_ID_PASSWORD = os.getenv("RDMNETBRKR_APPLE_DEVELOPER_ID_PW")

if not DEVEL_ID_USERNAME or not DEVEL_ID_PASSWORD:
    print("Couldn't get credentials to notarize application!")
    sys.exit(1)

###############################################################################
# Codesign
###############################################################################

print("Codesigning binaries...")

subprocess.run(
    [
        "security",
        "unlock-keychain",
        "-p",
        f"{KEYCHAIN_PASSWORD}",
        "/Users/gitlab-runner/Library/Keychains/login.keychain-db",
    ],
    check=True,
)

subprocess.run(
    [
        "codesign",
        "--force",
        "--sign",
        f"{MACOS_APPLICATION_SIGNING_IDENTITY}",
        "--timestamp",
        "-o",
        "runtime",
        "build/install/libexec/RDMnetBrokerService",
    ],
    check=True,
)

###############################################################################
# Build and sign pkg
###############################################################################

print("Building installer package...")
subprocess.run(["packagesbuild", "tools/install/macos/RDMnetBroker.pkgproj"], check=True)

print("Signing installer package...")
subprocess.run(
    [
        "productsign",
        "--sign",
        f"{MACOS_INSTALLER_SIGNING_IDENTITY}",
        "tools/install/macos/build/RDMnetBroker.pkg",
        "RDMnetBroker.pkg",
    ],
    check=True,
)
subprocess.run(["pkgutil", "--check-signature", "RDMnetBroker.pkg"], check=True)
