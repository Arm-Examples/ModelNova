/*
 * Copyright (c) 2026 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include "sysview_log.h"
#include "SEGGER_SYSVIEW.h"

#define MAX_LOG_MESSAGE_LENGTH  256

void sysview_log(const char* format, ...) {
  char buffer[MAX_LOG_MESSAGE_LENGTH];
  va_list args;

  va_start(args, format);
  vsnprintf(buffer, MAX_LOG_MESSAGE_LENGTH, format, args);
  va_end(args);

  SEGGER_SYSVIEW_Print(buffer);
}
