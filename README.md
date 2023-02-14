# RDMnet Broker

This repository contains an installable service for Mac and Windows which implements the functionality of an RDMnet Broker, as described in the ANSI E1.33 RDMnet standard.

For more information on RDMnet, see ETC's RDMnet
[repository](https://github.com/ETCLabs/RDMnet) and
[website](https://etclabs.github.io/RDMnet).

## About this ETCLabs Project

RDMnetBroker is official, open-source software developed by ETC employees and is designed to interact with ETC products. For challenges using, integrating, compiling, or modifying this software, we encourage posting on the [issues page](https://github.com/ETCLabs/RDMnetBroker/issues) of this project.

Before posting an issue or opening a pull request, please read the [contribution guidelines](./CONTRIBUTING.md).

## Quality Gates

### Code Reviews

* At least 2 developers must approve all code changes made before they can be merged into the integration branch.

### Automated testing

* This consists primarily of unit testing.

### Automated Style Checking

* Clang format is enabled – currently this follows the style guidelines established for our libraries, and it may be updated from time to time. See .clang-format for more details.
* Non-conformance to .clang-format will result in pipeline failures.  The code is not automatically re-formatted.

### Continuous Integration

* A GitLab CI pipeline is being used to run builds and tests that enforce all supported quality gates for all merge
requests, and for generating new binary builds from main. See .gitlab-ci.yml for details.

### Automated Dynamic Analysis

* When Linux is supported, ASAN will be used when running all automated tests to catch various memory errors during runtime.

## Revision Control

RDMnet Broker development is using Git for revision control.

## Installable Artifacts

Several artifacts are provided with each release to facilitate the installation of the broker service on Mac and Windows, either standalone or as part of your installer:

* Windows MSI installers for both x86 and x64, for standalone installation or execution from another installer.
* Windows merge modules for both x86 and x64 - use one of these to add the RDMnet broker service to your installer.
* Mac PKG installer, which can be used for standalone installation or added to another installer.

## Installation and Behavior

The broker can be installed either standalone or as part of another installer by using one of the artifacts listed above. On Windows, the broker is installed as a service that is configured to start automatically. On Mac, it's installed as a launchd daemon that, again, is configured to run automatically. The service will run as long as the host machine is running, on both platforms.

In addition, the following files and directories are installed on the system for configuration and logging:

* Windows configuration file path: `%PROGRAMDATA%\ETC\RDMnetBroker\Config\broker.conf`
* Windows log directory path: `%PROGRAMDATA%\ETC\RDMnetBroker\Logs`
* Mac configuration file path: `/usr/local/etc/RDMnetBroker/broker.conf`
* Mac log directory path: `/usr/local/var/log/RDMnetBroker`

The configuration file is monitored for changes by the broker service. The service will immediately restart when any change is detected. The configuration directory is configured on all platforms to allow modification without elevated permissions. This enables software to configure the broker service without elevated permissions.

The log directory contains rotating log files written by the broker service. The most recent log is named `broker.log`. When this log file is eventually rotated (i.e. when the service is stopped and restarted due to reboot, etc.), it will be renamed to `broker.log.1`, then `broker.log.2`, and so on, up to `broker.log.5`.

## Configuration

The broker service configuration file, `broker.conf`, contains a JSON object with various properties. Here is an example config with reasonable values for each property:

```json
{
  "enable_broker": true,

  "cid": "4958ac8f-cd5e-42cd-ab7e-9797b0efd3ac",
  "uid": {
    "type": "dynamic",
    "manufacturer_id": 25972
  },

  "dns_sd": {
    "service_instance_name": "My ETC RDMnet Broker",
    "manufacturer": "ETC",
    "model": "RDMnet Broker",
  },

  "scope": "default",
  "listen_port": 8888,
  "listen_interfaces": [
    "eth0",
    "wlan0"
  ],

  "log_level": "info",

  "max_connections": 20000,
  "max_controllers": 1000,
  "max_controller_messages": 500,
  "max_devices": 20000,
  "max_device_messages": 500,
  "max_reject_connections": 1000
}
```

Properties that are not present, out of range, or otherwise invalid are assigned a reasonable default.

What follows is an overview of the currently supported properties and how to set them.

### Enable Broker

This is a boolean property that determines whether the service's broker functionality should be active or not. For example, to disable broker functionality:

```json
  "enable_broker": false
```

When broker functionality is disabled, the service remains running and continues monitoring the configuration file for further changes. However, the service will not actually run a broker until configured to do so.

### CID

This is a string property that should be set to the desired CID for the broker. Enter the CID as a UUID without curly braces, for example:

```json
  "cid": "4958ac8f-cd5e-42cd-ab7e-9797b0efd3ac"
```

### UID

This determines what UID the broker should have. The UID can be either dynamic or static. If dynamic, the device ID will be assigned dynamically by the broker. Otherwise if it’s static, then the device ID is specified along with the manufacturer ID.

Here’s an example of how to specify a dynamic broker UID:

```json
  "uid": {
    "type": "dynamic",
    "manufacturer_id": 25972
  }
```

Here’s an example of how to specify a static broker UID:

```json
  "uid": {
    "type": "static",
    "manufacturer_id": 25972,
    "device_id": 123
  }
```

### DNS-SD

Various DNS-SD settings can be configured. The service instance name, manufacturer, and model are all strings that are used for various DNS-SD components. For example:

```json
  "dns_sd": {
    "service_instance_name": "My ETC RDMnet Broker",
    "manufacturer": "ETC",
    "model": "RDMnet Broker",
  }
```

### Scope

The scope of the broker can be configured as a string property:

```json
  "scope": "default"
```

### Listen Port

The port the broker will use for connections can be configured as a number:

```json
  "listen_port": 8888
```

### Listen Interfaces

The broker can be configured to use specific network interfaces. If nothing is specified in the config, all interfaces will be used. Here’s an example of specifying interfaces on Mac:

```json
  "listen_interfaces": [
    "eth0",
    "wlan0"
  ]
```

On Mac, the names of the interfaces are used - to find the names of the interfaces, use a command prompt and `/sbin/ifconfig`.

On Windows, GUIDs are used instead:

```json
  "listen_interfaces": [
    "{D1DB0425-03FB-81BB-8120-0787286B3EA6}",
    "{B33CDECD-229B-467A-BC63-54F2073A31CE}"
  ]
```

To find the GUIDs, navigate to this registry location in RegEdit: `Computer\HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\NetworkCards`.

Each folder is named with a number, and each individually represents a network interface. Look for the key `ServiceName` – the value will be the GUID of the interface. Just copy that into the configuration file as the string value, as shown above.

### Log Level

The log level can be set as a string. All log messages at this level or above will be logged. Example:

```json
  "log_level": "info"
```

The allowed strings for this property are `debug`, `info`, `notice`, `warning`, `err`, `crit`, `alert`, and `emerg`.

### Maximums

Various configuration properties are available for setting various limits.

#### Max Connections

The maximum number of client connections supported. 0 means infinite:

```json
  "max_connections": 20000
```

#### Max Controllers

The maximum number of controllers allowed. 0 means infinite:

```json
  "max_controllers": 1000
```

#### Max Controller Messages

The maximum number of queued messages per controller. 0 means infinite:

```json
  "max_controller_messages": 500
```

#### Max Devices

The maximum number of devices allowed. 0 means infinite:

```json
  "max_devices": 20000
```

#### Max Device Messages

The maximum number of queued messages per device. 0 means infinite:

```json
  "max_device_messages": 500
```

#### Max Reject Connections

If you reach the number of max connections, this number of TCP-level connections are still supported to reject the connection request:

```json
  "max_reject_connections": 1000
```

## License

RDMnet Broker is licensed under the Apache License 2.0. RDMnet Broker also incorporates the [RDMnet](https://github.com/ETCLabs/RDMnet) library, which has additional licensing terms.
