/*
 * Copyright (c) 2025-2026 Arm Limited. All rights reserved.
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

#include <cstddef>  // For size_t
#include <cstdint>  // For uint8_t, int32_t, uint32_t
#include <cstdio>   // For printf
#include <stdio.h>
#include <string.h>

#include "cmsis_vstream.h"

#include "sds_algorithm_config.h"
#include "sds_algorithm.h"

#include "app_setup.h"
#include "image_processing_func.h"

/* Display frame buffer (RGB888) */
static uint8_t LCD_Frame[DISPLAY_IMAGE_SIZE] DISPLAY_FRAME_BUF_ATTRIBUTE;

/* Tensor arena buffer */
static uint8_t tensorArena[ACTIVATION_BUF_SZ] ACTIVATION_BUF_ATTRIBUTE;

/* Input image dimensions */
int inputImgCols;
int inputImgRows;

/* Reference to the underlying CMSIS vStream VideoOut driver */
extern vStreamDriver_t          Driver_vStreamVideoOut;
#define vStream_VideoOut      (&Driver_vStreamVideoOut)


/* Video Out Stream Event Callback */
void VideoOut_Event_Callback (uint32_t event) {
  (void)event;
}

/**
  \fn           int32_t InitAlgorithm (void)
  \brief        Initialize algorithm under test.
  \return       0 on success; -1 on error
*/
int32_t InitAlgorithm (void) {
  vStreamStatus_t status;

  /* Initialize Video Output Stream */
  if (vStream_VideoOut->Initialize(VideoOut_Event_Callback) != VSTREAM_OK) {
      printf("Failed to initialise video output driver\n");
      return -1;
  }

  /* Set Output Video buffer */
  if (vStream_VideoOut->SetBuf(LCD_Frame, sizeof(LCD_Frame), DISPLAY_IMAGE_SIZE) != VSTREAM_OK) {
      printf("Failed to set buffer for video output\n");
      return -1;
  }

  return 0;
}