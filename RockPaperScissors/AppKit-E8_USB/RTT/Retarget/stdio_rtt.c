/*---------------------------------------------------------------------------
 * Copyright (c) 2026 Arm Limited (or its affiliates).
 * All rights reserved.
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
 *
 *      Name:    stdio_rtt.c
 *      Purpose: Retarget stdio to Segger RTT
 *---------------------------------------------------------------------------*/

#include <stdint.h>
#ifdef   CMSIS_target_header
#include CMSIS_target_header
#endif

#include "SEGGER_RTT.h"

// Exported functions
extern int stdio_init     (void);
extern int stderr_putchar (int ch);
extern int stdout_putchar (int ch);
extern int stdin_getchar  (void);

/**
  Initialize stdio

  \return          0 on success, or -1 on error.
*/
int stdio_init (void) {

//SEGGER_RTT_Init();  // Not explicitly required, as RTT is initialized implicitly on first use

  return 0;
}

/**
  Put a character to the stderr

  \param[in]   ch  Character to output
  \return          The character written, or -1 on write error.
*/
int stderr_putchar (int ch) {
  unsigned res;

  res = SEGGER_RTT_PutChar(0U, (char)ch);

  if (res != 1U) {
    return -1;
  }

  return ch;
}

/**
  Put a character to the stdout

  \param[in]   ch  Character to output
  \return          The character written, or -1 on write error.
*/
int stdout_putchar (int ch) {
  unsigned res;

  res = SEGGER_RTT_PutChar(0U, (char)ch);

  if (res != 1U) {
    return -1;
  }

  return ch;
}

/**
  Get a character from the stdio

  \return     The next character from the input, or -1 on read error.
*/
int stdin_getchar (void) {
  int ret;

  ret = SEGGER_RTT_WaitKey();

  return ret;
}
