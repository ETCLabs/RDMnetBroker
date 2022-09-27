# RDMnet Broker

This repository contains an executable which implements the functionality of an
RDMnet Broker, as described in the ANSI E1.33 RDMnet standard.

For more information on RDMnet, see ETC's RDMnet
[repository](https://github.com/ETCLabs/RDMnet) and
[website](https://etclabs.github.io/RDMnet).

## About this ETCLabs Project

RDMnetBroker is official, open-source software developed by ETC employees and
is designed to interact with ETC products. For challenges using, integrating,
compiling, or modifying this software, we encourage posting on the
[issues page](https://github.com/ETCLabs/RDMnetBroker/issues) of this project.

Before posting an issue or opening a pull request, please read the
[contribution guidelines](./CONTRIBUTING.md).

## Quality Gates

### Code Reviews

* At least 2 developers must approve all code changes made before they can be merged into the integration branch.

### Automated testing

* This consists primarily of unit testing.

### Automated Style Checking

* Clang format is enabled – currently this follows the style guidelines established for our libraries,
 and it may be updated from time to time. See .clang-format for more details.

### Continuous Integration

* A GitLab CI pipeline is being used to run builds and tests that enforce all supported quality gates for all merge
requests, and for generating new binary builds from main. See .gitlab-ci.yml for details.

### Automated Dynamic Analysis

* When Linux is supported, ASAN will be used when running all automated tests to catch various memory errors during runtime.

## Revision Control

RDMnet Broker development is using Git for revision control.

## License

RDMnet Broker is licensed under the Apache License 2.0. RDMnet Broker also incorporates the [RDMnet](https://github.com/ETCLabs/RDMnet) library, which has additional licensing terms.
