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

#include "broker_common.h"
#include "etcpal/cpp/log.h"
#include <cassert>

class AssertLogHandler : public etcpal::LogMessageHandler
{
public:
  AssertLogHandler(const std::function<void(const char*)>& log_fn) : log_fn_(log_fn) {}

  void HandleLogMessage(const EtcPalLogStrings& strings) override { log_fn_(strings.raw); }

private:
  const std::function<void(const char*)>& log_fn_;
};

class AssertLogger
{
public:
  AssertLogger(const std::function<void(const char*)>& log_fn) : log_handler_(log_fn) { logger_.Startup(log_handler_); }
  ~AssertLogger() { logger_.Shutdown(); }

private:
  etcpal::Logger   logger_;
  AssertLogHandler log_handler_;
}

bool AssertVerifyFail(const char*                             exp,
                      const char*                             file,
                      const char*                             func,
                      int                                     line,
                      const std::function<void(const char*)>& log_fn)
{
  AssertLogger logger(log_fn);
  logger.Critical(R"(ASSERTION "%s" FAILED (FILE: "%s" FUNCTION: "%s" LINE: %d))", exp ? exp : "", file ? file : "",
                  func ? func : "", line);

  assert(false);
  return false;
}
