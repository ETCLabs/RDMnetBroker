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

#ifndef BROKER_COMMON_H_
#define BROKER_COMMON_H_

#include "etcpal/cpp/log.h"

bool AssertVerifyFail(const char*                             exp,
                      const char*                             file,
                      const char*                             func,
                      int                                     line,
                      const std::function<void(const char*)>& log_fn);

#define BROKER_ASSERT_VERIFY(exp, log_fn) \
  ((exp) ? true : (AssertVerifyFail(#exp, __FILE__, static_cast<const char*>(__func__), __LINE__, log_fn) && false))

#endif /* BROKER_COMMON_H_ */
