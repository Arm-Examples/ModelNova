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

#ifndef SYSVIEW_LOG_H_
#define SYSVIEW_LOG_H_

#if ET_LOG_ENABLED
#define ET_LOG(_level, _format, ...)                                 \
  do {                                                               \
    sysview_log(_format,##__VA_ARGS__);                              \
  } while (0)
#else
#define ET_LOG(_level, _format, ...) ((void)0)
#endif

#ifdef __cplusplus
extern "C" {
#endif

void sysview_log(const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif /* SYSVIEW_LOG_H_ */
