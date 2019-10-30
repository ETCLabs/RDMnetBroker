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

/// \file windows/service_utils.h
/// \brief Windows NT Service utility functions

#ifndef SERVICE_UTILS_H_
#define SERVICE_UTILS_H_

#include <winsock2.h>
#include <windows.h>

void GetLastErrorMessage(wchar_t* msg_buf_out, size_t buf_size);
void GetLastErrorMessage(DWORD code, wchar_t* msg_buf_out, size_t buf_size);

void InstallService(const wchar_t* service_name, const wchar_t* display_name, const wchar_t* description,
                    DWORD start_type, const wchar_t* dependencies);
void UninstallService(const wchar_t* service_name);

#endif  // SERVICE_UTILS_H_
