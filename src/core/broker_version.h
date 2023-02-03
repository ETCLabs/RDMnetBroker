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

///////////////////////////////////////////////////////////////////////////////////////////////////
// IMPORTANT
///////////////////////////////////////////////////////////////////////////////////////////////////
// If you make changes to this file, you must make a new copy of it in
// tools/version/templates/broker_version.h.in
///////////////////////////////////////////////////////////////////////////////////////////////////
// broker_version.h: Provides the version information for the RDMnet Broker Service

#ifndef BROKER_VERSION_H_
#define BROKER_VERSION_H_

#include <string>
#include <sstream>

// clang-format off

struct BrokerVersion
{
  static constexpr int kVersionMajor = 1;
  static constexpr int kVersionMinor = 0;
  static constexpr int kVersionPatch = 0;
  static constexpr int kVersionBuild = 2;

  static std::string VersionString();
  static std::string ProductNameString();
  static std::string BuildDateString();
  static std::string CopyrightString();
};

inline std::string BrokerVersion::VersionString()
{
  std::stringstream res;
  res << kVersionMajor << '.' << kVersionMinor << '.' << kVersionPatch << '.' << kVersionBuild;
  return res.str();
}

inline std::string BrokerVersion::ProductNameString()
{
  return "ETC RDMnet Broker Service";
}

inline std::string BrokerVersion::BuildDateString()
{
  return "03.Feb.2023";
}

inline std::string BrokerVersion::CopyrightString()
{
  return "Copyright 2023 ETC Inc.";
}

#endif  // BROKER_VERSION_H_
