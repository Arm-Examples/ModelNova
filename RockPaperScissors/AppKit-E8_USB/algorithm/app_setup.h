
#ifndef APP_SETUP_H__
#define APP_SETUP_H__

#include "config_app.h"
#include "config_video.h"
#include "config_ml_model.h"

/* Attributes applied to the activation buffer object */
#define ACTIVATION_BUF_ATTRIBUTE \
  __attribute__((section(ACTIVATION_BUF_SECTION), aligned(ACTIVATION_BUF_ALIGNMENT)))

/* Attributes applied to the ML image buffer object */
#define NN_MODEL_BUF_ATTRIBUTE \
  __attribute__((section(NN_MODEL_BUF_SECTION), aligned(NN_MODEL_BUF_ALIGNMENT)))

#define MODEL_TFLITE_ATTRIBUTE  NN_MODEL_BUF_ATTRIBUTE

/* Attributes applied to the camera frame buffer object */
#define CAMERA_FRAME_BUF_ATTRIBUTE \
  __attribute__((section(CAMERA_FRAME_BUF_SECTION), aligned(CAMERA_FRAME_BUF_ALIGNMENT)))

/* Attributes applied to the RGB image buffer object */
#define RGB_IMAGE_BUF_ATTRIBUTE \
  __attribute__((section(RGB_IMAGE_BUF_SECTION), aligned(RGB_IMAGE_BUF_ALIGNMENT)))

/* Attributes applied to the ML image buffer object */
#define ML_IMAGE_BUF_ATTRIBUTE \
  __attribute__((section(ML_IMAGE_BUF_SECTION), aligned(ML_IMAGE_BUF_ALIGNMENT)))

/* Attributes applied to the display frame buffer object */
#define DISPLAY_FRAME_BUF_ATTRIBUTE \
  __attribute__((section(DISPLAY_FRAME_BUF_SECTION), aligned(DISPLAY_FRAME_BUF_ALIGNMENT)))


  /* Frame type */
#define CAMERA_FRAME_TYPE_RAW8    0U
#define CAMERA_FRAME_TYPE_RGB565  1U
#define CAMERA_FRAME_TYPE_RGB888  2U

/* Define input image bit depth */
#if (CAMERA_FRAME_TYPE == CAMERA_FRAME_TYPE_RAW8)
#define CAMERA_FRAME_COLOR_BYTES 1
#elif (CAMERA_FRAME_TYPE == CAMERA_FRAME_TYPE_RGB565)
#define CAMERA_FRAME_COLOR_BYTES 2
#elif (CAMERA_FRAME_TYPE == CAMERA_FRAME_TYPE_RGB888)
#define CAMERA_FRAME_COLOR_BYTES 3
#else
#error "Camera frame type not supported, check CAMERA_FRAME_TYPE definition."
#endif

/* Ensure that the RGB image is square and smaller than camera frame */
#if (RGB_IMAGE_WIDTH != RGB_IMAGE_HEIGHT)   || \
    (RGB_IMAGE_WIDTH > CAMERA_FRAME_WIDTH)  || \
    (RGB_IMAGE_WIDTH > CAMERA_FRAME_HEIGHT) || \
    (RGB_IMAGE_HEIGHT > CAMERA_FRAME_WIDTH) || \
    (RGB_IMAGE_HEIGHT > CAMERA_FRAME_HEIGHT)
#error "RGB image must be smaller than camera frame, check RGB_IMAGE_WIDTH and RGB_IMAGE_HEIGHT definitions."
#endif

/* Number of bytes per pixel for RGB buffer (RGB888) */
#define RGB_IMAGE_COLOR_BYTES       3

/* Number of bytes per pixel for ML image */
#define ML_IMAGE_COLOR_BYTES        3

/* Number of bytes per pixel for display */
#define DISPLAY_FRAME_COLOR_BYTES   3


/* Define camera RAW frame size */
#define CAMERA_FRAME_SIZE      (CAMERA_FRAME_WIDTH * CAMERA_FRAME_HEIGHT * CAMERA_FRAME_COLOR_BYTES)

/* Define RGB image size (RGB888) */
#define RGB_IMAGE_SIZE         (RGB_IMAGE_WIDTH * RGB_IMAGE_HEIGHT * RGB_IMAGE_COLOR_BYTES)

/* Define ML image size */
#define ML_IMAGE_SIZE          (ML_IMAGE_WIDTH * ML_IMAGE_HEIGHT * ML_IMAGE_COLOR_BYTES)

/* Define display image size */
#define DISPLAY_IMAGE_SIZE     (DISPLAY_FRAME_WIDTH * DISPLAY_FRAME_HEIGHT * DISPLAY_FRAME_COLOR_BYTES)

#endif /* APP_SETUP_H__ */