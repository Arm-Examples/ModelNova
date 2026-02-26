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

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "cmsis_os2.h"
#include "cmsis_vstream.h"

#include "sds_algorithm_config.h"
#include "sds_data_in.h"

#include "app_setup.h"
#include "image_processing_func.h"

#ifdef USE_SEGGER_SYSVIEW
#include "SEGGER_SYSVIEW.h"
#include "sysview_markers.h"
#endif

/* Reference to the underlying CMSIS vStream VideoIn driver */
extern vStreamDriver_t          Driver_vStreamVideoIn;
#define vStream_VideoIn       (&Driver_vStreamVideoIn)

/* Reference to the underlying CMSIS vStream VideoOut driver */
extern vStreamDriver_t          Driver_vStreamVideoOut;
#define vStream_VideoOut      (&Driver_vStreamVideoOut)

/* Camera frame buffer (RAW8 or RGB565) */
static uint8_t CAM_Frame[CAMERA_FRAME_SIZE] CAMERA_FRAME_BUF_ATTRIBUTE;

/* RGB image buffer (RGB888) */
static uint8_t RGB_Image[RGB_IMAGE_SIZE] RGB_IMAGE_BUF_ATTRIBUTE;

static void convert_frame_to_rgb(uint8_t *inFrame);

/* Algorithm thread ID */
osThreadId_t tid_algo = NULL;

/* Video In Stream Event Callback */
void VideoIn_Event_Callback (uint32_t event) {

  if (event & VSTREAM_EVENT_DATA) {
    /* Video frame is available in camera frame buffer */
    osThreadFlagsSet(tid_algo, 0x1);
  }
}

/* Video Out Stream Event Callback */
void VideoOut_Event_Callback (uint32_t event) {
  (void)event;
}

/**
  \fn           int32_t InitInputData (void)
  \brief        Initialize system for acquiring input data.
  \return       0 on success; -1 on error
*/
int32_t InitInputData (void) {

  tid_algo = osThreadGetId();

  /* Initialize Video Input Stream */
  if (vStream_VideoIn->Initialize(VideoIn_Event_Callback) != VSTREAM_OK) {
    printf("Failed to initialise video input driver\n");
    return -1;
  }

  /* Set Input Video buffer */
  if (vStream_VideoIn->SetBuf(CAM_Frame, sizeof(CAM_Frame), CAMERA_FRAME_SIZE) != VSTREAM_OK) {
    printf("Failed to set buffer for video input\n");
    return -1;
  }

  return 0;
}

/**
  \fn           int32_t GetInputData (uint8_t *buf, uint32_t max_len)
  \brief        Get input data block as required for algorithm under test.
  \details      Size of this block has to match size expected by algorithm under test.
  \param[out]   buf             pointer to memory buffer for acquiring input data
  \param[in]    max_len         maximum number of bytes of input data to acquire
  \return       number of data bytes returned; -1 on error
*/
int32_t GetInputData (uint8_t *buf, uint32_t max_len) {
  uint8_t *inFrame;

  // Check input parameters
  if ((buf == NULL) || (max_len == 0U)) {
    return -1;
  }

  // Check if buffer can fit expected data
  if (max_len < SDS_ALGO_DATA_IN_BLOCK_SIZE) {
    return -1;
  }

#ifdef USE_SEGGER_SYSVIEW
    SEGGER_SYSVIEW_MarkStart(SYSVIEW_MARKER_CAPTURE_IMAGE);
#endif

  /* Start video capture */
  if (vStream_VideoIn->Start(VSTREAM_MODE_SINGLE) != VSTREAM_OK) {
    printf("Failed to start video capture\n");
    return -1;
  }

  /* Wait for new video input frame */
  osThreadFlagsWait(0x1, osFlagsWaitAny, osWaitForever);

#ifdef USE_SEGGER_SYSVIEW
    SEGGER_SYSVIEW_MarkStop(SYSVIEW_MARKER_CAPTURE_IMAGE);
#endif

    /* Get input video frame buffer */
  inFrame = (uint8_t *)vStream_VideoIn->GetBlock();
  if (inFrame == NULL) {
    printf("Failed to get video input frame\n");
    return -1;
  }

#ifdef USE_SEGGER_SYSVIEW
    SEGGER_SYSVIEW_MarkStart(SYSVIEW_MARKER_CONVERT_IMAGE);
#endif

  /* Convert input frame and place it into RGB_Image buffer */
  convert_frame_to_rgb(inFrame);

#ifdef USE_SEGGER_SYSVIEW
    SEGGER_SYSVIEW_MarkStop(SYSVIEW_MARKER_CONVERT_IMAGE);
#endif

#ifdef USE_SEGGER_SYSVIEW
    SEGGER_SYSVIEW_MarkStart(SYSVIEW_MARKER_RESIZE_IMAGE);
#endif

  /* Resize RGB image to fit ML model expected size */
  crop_resize_rgb565_to_rgb888(inFrame,
                               CAMERA_FRAME_WIDTH,
                               CAMERA_FRAME_HEIGHT,
                               buf,
                               ML_IMAGE_WIDTH,
                               ML_IMAGE_HEIGHT);

#ifdef USE_SEGGER_SYSVIEW
    SEGGER_SYSVIEW_MarkStop(SYSVIEW_MARKER_RESIZE_IMAGE);
#endif

  /* Release input frame */
  if (vStream_VideoIn->ReleaseBlock() != VSTREAM_OK) {
    printf("Failed to release video input frame\n");
  }

  return SDS_ALGO_DATA_IN_BLOCK_SIZE;
}

/*
  Converts camera frame and copies it to RGB image buffer.

  Camera frame may be square or non-square and must be in RAW8 or RGB565 format.
  RGB image buffer is always square and is in RGB888 format.

  The function handles the following cases:
    - If the camera frame is square and matches the RGB image size:
      - crop and debayer the RAW8 camera frame
      - convert RGB565 camera frame to RGB888
      - copy RGB888 camera frame to RGB image buffer.
    - If the camera frame is square and larger than the RGB image size:
      - crop and debayer the RAW8 camera frame
      - resize RGB565 camera frame to fit into RGB image buffer.
      - resize RGB888 camera frame to fit into RGB image buffer.
    - If the camera frame is not square:
      - crop and debayer the RAW8 camera frame
      - crop RGB565 camera frame to fit into RGB image buffer.
      - crop RGB888 camera frame to fit into RGB image buffer.
*/
static void convert_frame_to_rgb(uint8_t *inFrame) {
    #if (CAMERA_FRAME_WIDTH == CAMERA_FRAME_HEIGHT)
      /* Camera frame is square */
      #if (CAMERA_FRAME_WIDTH == RGB_IMAGE_WIDTH) && (CAMERA_FRAME_HEIGHT == RGB_IMAGE_HEIGHT)
        /* Camera frame size matches RGB image size */
        #if (CAMERA_FRAME_TYPE == CAMERA_FRAME_TYPE_RAW8)
            /* For RAW8, crop and debayer into RGB image buffer (RGB888) */
            crop_and_debayer(inFrame,
                            CAMERA_FRAME_WIDTH,
                            CAMERA_FRAME_HEIGHT,
                            0, 0, /* Crop from top-left corner */
                            RGB_Image,
                            RGB_IMAGE_WIDTH,
                            RGB_IMAGE_HEIGHT,
                            CAMERA_FRAME_BAYER);
        #elif (CAMERA_FRAME_TYPE == CAMERA_FRAME_TYPE_RGB565)
            /* For RGB565, convert frame to fit into RGB image buffer (RGB888) */
            convert_rgb565_to_rgb888(inFrame, RGB_Image, CAMERA_FRAME_WIDTH, CAMERA_FRAME_HEIGHT);
        #elif (CAMERA_FRAME_TYPE == CAMERA_FRAME_TYPE_RGB888)
            /* For RGB888, just copy the frame */
            memcpy(RGB_Image, inFrame, CAMERA_FRAME_WIDTH * CAMERA_FRAME_HEIGHT * 3);
        #endif
      #else
        /* Camera frame size is larger than RGB image size */
        #if (CAMERA_FRAME_TYPE == CAMERA_FRAME_TYPE_RAW8)
            /* For RAW8, crop and debayer into RGB image buffer (RGB888) */
            crop_and_debayer(inFrame,
                            CAMERA_FRAME_WIDTH,
                            CAMERA_FRAME_HEIGHT,
                            (CAMERA_FRAME_WIDTH - RGB_IMAGE_WIDTH) / 2, /* Center crop */
                            (CAMERA_FRAME_HEIGHT - RGB_IMAGE_HEIGHT) / 2,
                            RGB_Image,
                            RGB_IMAGE_WIDTH,
                            RGB_IMAGE_HEIGHT,
                            CAMERA_FRAME_BAYER);
        #elif (CAMERA_FRAME_TYPE == CAMERA_FRAME_TYPE_RGB565)
            /* For RGB565, resize frame to fit into RGB image buffer (RGB888) */
            image_resize(inFrame,
                        CAMERA_FRAME_WIDTH,
                        CAMERA_FRAME_HEIGHT,
                        RGB_Image,
                        RGB_IMAGE_WIDTH,
                        RGB_IMAGE_HEIGHT,
                        IMAGE_FORMAT_RGB565,
                        IMAGE_FORMAT_RGB888);
        #elif (CAMERA_FRAME_TYPE == CAMERA_FRAME_TYPE_RGB888)
            /* For RGB888, resize frame to fit into RGB image buffer (RGB888) */
            image_resize(inFrame,
                        CAMERA_FRAME_WIDTH,
                        CAMERA_FRAME_HEIGHT,
                        RGB_Image,
                        RGB_IMAGE_WIDTH,
                        RGB_IMAGE_HEIGHT,
                        IMAGE_FORMAT_RGB888,
                        IMAGE_FORMAT_RGB888);
        #endif
      #endif
    #endif

    #if (CAMERA_FRAME_WIDTH != CAMERA_FRAME_HEIGHT)
      /* Camera frame is not square, crop it to fit RGB buffer */
      #if (CAMERA_FRAME_TYPE == CAMERA_FRAME_TYPE_RAW8)
        /* For RAW8, crop and debayer to RGB888 */
        crop_and_debayer(inFrame,
                        CAMERA_FRAME_WIDTH,
                        CAMERA_FRAME_HEIGHT,
                        (CAMERA_FRAME_WIDTH - RGB_IMAGE_WIDTH) / 2, /* Center crop */
                        (CAMERA_FRAME_HEIGHT - RGB_IMAGE_HEIGHT) / 2,
                        RGB_Image,
                        RGB_IMAGE_WIDTH,
                        RGB_IMAGE_HEIGHT,
                        CAMERA_FRAME_BAYER);
      #elif (CAMERA_FRAME_TYPE == CAMERA_FRAME_TYPE_RGB565)
        /* For RGB565, crop and convert to RGB888 */
        crop_rgb565_to_rgb888(inFrame,
                            CAMERA_FRAME_WIDTH,
                            CAMERA_FRAME_HEIGHT,
                            RGB_Image,
                            (CAMERA_FRAME_WIDTH - RGB_IMAGE_WIDTH) / 2, /* Center crop */
                            (CAMERA_FRAME_HEIGHT - RGB_IMAGE_HEIGHT) / 2,
                            RGB_IMAGE_WIDTH,
                            RGB_IMAGE_HEIGHT);
      #elif (CAMERA_FRAME_TYPE == CAMERA_FRAME_TYPE_RGB888)
        /* For RGB888, just crop */
        crop_rgb888_to_rgb888(inFrame,
                            CAMERA_FRAME_WIDTH,
                            CAMERA_FRAME_HEIGHT,
                            RGB_Image,
                            (CAMERA_FRAME_WIDTH - RGB_IMAGE_WIDTH) / 2, /* Center crop */
                            (CAMERA_FRAME_HEIGHT - RGB_IMAGE_HEIGHT) / 2,
                            RGB_IMAGE_WIDTH,
                            RGB_IMAGE_HEIGHT);
      #endif
    #endif
}
