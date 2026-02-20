/*---------------------------------------------------------------------------
 * Copyright (c) 2025 Arm Limited (or its affiliates). All rights reserved.
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
 *---------------------------------------------------------------------------*/

#ifndef IMAGE_PROCESSING_FUNC_H__
#define IMAGE_PROCESSING_FUNC_H__

#include <stdint.h>

/* Bayer pattern definitions */
#define BAYER_PATTERN_RGGB      0
#define BAYER_PATTERN_BGGR      1
#define BAYER_PATTERN_GRBG      2
#define BAYER_PATTERN_GBRG      3

/* Image format definitions */
#define IMAGE_FORMAT_GRAYSCALE  0   ///< 8-bit grayscale: 1 byte per pixel
#define IMAGE_FORMAT_RGB565     1   ///< 16-bit RGB: 5 bits R, 6 bits G, 5 bits B
#define IMAGE_FORMAT_RGB888     2   ///< 24-bit RGB: 8 bits each for R, G, B

#ifdef __cplusplus
extern "C" {
#endif

typedef int bayer_pattern_t;
typedef int image_format_t;

/**
 * @brief Perform debayering on a raw Bayer image.
 *
 * Converts a single-channel Bayer-pattern image into a full RGB image.
 *
 * The raw Bayer image must follow one of the standard 2x2 Bayer patterns and
 * use 8-bit grayscale pixels. The output RGB image is stored as 24-bit RGB
 * (3 bytes per pixel, in R-G-B order). Supports optional red/blue channel swap.
 *
 * Note: The outer 1-pixel border is skipped to avoid accessing out-of-bounds
 * pixels. These edges will remain unprocessed unless handled separately.
 *
 * @param[in]  raw      Pointer to the raw Bayer image buffer (size: width × height).
 * @param[out] rgb      Pointer to the output RGB buffer (size: width × height × 3).
 * @param[in]  width    Width of the image in pixels.
 * @param[in]  height   Height of the image in pixels.
 * @param[in]  pattern  Bayer pattern used in the raw image.
 * @param[in]  swap_rb  If non-zero, swap the red and blue channels in the output.
 */
void image_debayer(const uint8_t *raw,
                   uint8_t *rgb,
                   int width,
                   int height,
                   bayer_pattern_t pattern,
                   int swap_rb);

/**
 * @brief Crop a region from a RAW8 Bayer image and convert it to RGB888 with bilinear debayering and scaling.
 *
 * This function performs the following in one pass:
 * 1. Crops a subregion from a RAW8 Bayer-patterned image.
 * 2. Scales the cropped region to a specified output resolution.
 * 3. Performs bilinear interpolation to reconstruct full-color RGB pixels.
 *
 * @param[in]  src           Pointer to the input RAW8 image buffer.
 * @param[in]  src_width     Width of the input RAW8 image in pixels.
 * @param[in]  src_height    Height of the input RAW8 image in pixels.
 * @param[in]  src_crop_x    X offset of the top-left corner of the crop region.
 * @param[in]  src_crop_y    Y offset of the top-left corner of the crop region.
 * @param[out] dst_rgb       Pointer to the output RGB888 buffer. Must be at least dst_width * dst_height * 3 bytes.
 * @param[in]  dst_width     Width of the output image in pixels.
 * @param[in]  dst_height    Height of the output image in pixels.
 * @param[in]  pattern       Bayer pattern used in the RAW8 image (see @ref bayer_pattern_t).
 */
void crop_and_debayer(const uint8_t *src,
                      int src_width,
                      int src_height,
                      int src_crop_x,
                      int src_crop_y,
                      uint8_t *dst_rgb,
                      int dst_width,
                      int dst_height,
                      bayer_pattern_t pattern);

/**
 * @brief Center-crop and resize an RGB565 image to RGB888 format.
 *
 * This function performs a center crop of the source image to a square region
 * (based on the source height), then resizes it to the specified destination
 * resolution using fixed-point interpolation. During resizing, pixel format
 * conversion from RGB565 to RGB888 is performed.
 *
 * Processing steps:
 * - A centered square crop is computed from the source image.
 * - The cropped region is resized to the destination resolution.
 * - RGB565 pixels are unpacked and expanded to RGB888 format.
 *
 * The operation is performed in a single pass without allocating any
 * intermediate buffers, making it memory efficient for embedded systems.
 *
 * @param src         Pointer to the source image buffer (RGB565 format).
 * @param src_width   Width of the source image in pixels.
 * @param src_height  Height of the source image in pixels.
 * @param dst         Pointer to the destination image buffer (RGB888 format).
 * @param dst_width   Width of the destination image in pixels.
 * @param dst_height  Height of the destination image in pixels.
 *
 * @note The source image must be in RGB565 format (2 bytes per pixel).
 * @note The destination buffer must be preallocated with at least
 *       `dst_width * dst_height * 3` bytes.
 * @note This function assumes src_width >= src_height for proper center cropping.
 */
void crop_resize_rgb565_to_rgb888(const uint8_t *src,
                                  int src_width,
                                  int src_height,
                                  uint8_t *dst,
                                  int dst_width,
                                  int dst_height);

/**
 * @brief Resize an image with format conversion.
 *
 * This function resizes an input image to a new resolution. It supports different image
 * formats for the source and destination, and automatically handles color conversion
 * between grayscale, RGB565, and RGB888 formats.
 *
 * Format handling:
 * - Grayscale is treated as luminance; RGB conversions are done via luminance (Y = 0.299R + 0.587G + 0.114B).
 * - RGB565 is unpacked and interpolated as RGB888 for better accuracy.
 *
 * @param src         Pointer to the source image buffer.
 * @param src_width   Width of the source image in pixels.
 * @param src_height  Height of the source image in pixels.
 * @param dst         Pointer to the destination image buffer.
 * @param dst_width   Width of the destination image in pixels.
 * @param dst_height  Height of the destination image in pixels.
 * @param src_format  Format of the source image (GRAYSCALE, RGB565, or RGB888).
 * @param dst_format  Format of the destination image (GRAYSCALE, RGB565, or RGB888).
 *
 * @note The destination buffer must be preallocated with enough space to hold
 *       `dst_width * dst_height * dst_bpp` bytes, where `dst_bpp` depends on the format:
 *       - GRAYSCALE: 1 byte per pixel
 *       - RGB565:    2 bytes per pixel
 *       - RGB888:    3 bytes per pixel
 */
void image_resize(const uint8_t *src,
                  int src_width,
                  int src_height,
                  uint8_t *dst,
                  int dst_width,
                  int dst_height,
                  image_format_t src_format,
                  image_format_t dst_format);

/**
 * @brief Copy a smaller or equally sized image into a destination frame buffer at a given offset.
 *
 * Assumes that the source image fits completely within the destination at the specified offset.
 * It assumes both source and destination use the same format (grayscale, RGB565, or RGB888).
 *
 * @param src        Pointer to the source image buffer.
 * @param src_width  Width of the source image in pixels.
 * @param src_height Height of the source image in pixels.
 * @param dst        Pointer to the destination framebuffer buffer.
 * @param dst_width  Width of the destination framebuffer in pixels.
 * @param dst_height Height of the destination framebuffer in pixels.
 * @param x_offset   X offset in the framebuffer where the image should be placed.
 * @param y_offset   Y offset in the framebuffer where the image should be placed.
 * @param format     Pixel format for both source and destination (must match).
 */
void image_copy_to_framebuffer(const uint8_t *src,
                               int src_width,
                               int src_height,
                               uint8_t *dst,
                               int dst_width,
                               int dst_height,
                               int x_offset,
                               int y_offset,
                               image_format_t format,
                               int flip_horizontal,
                               int flip_vertical,
                               int swap_rb);


/**
 * @brief Convert an RGB565 image to RGB888 format.
 *
 * This function takes a source image buffer in RGB565 format (2 bytes per pixel)
 * and converts it to RGB888 format (3 bytes per pixel). The width and height
 * define the dimensions of the image.
 *
 * @param src         Pointer to the input RGB565 image buffer.
 * @param dst         Pointer to the output RGB888 image buffer.
 * @param width       Width of the image in pixels.
 * @param height      Height of the image in pixels.
 */
void convert_rgb565_to_rgb888(const uint8_t *src,
                              uint8_t *dst,
                              int width,
                              int height);

/**
 * @brief Crop a region from an RGB565 image and convert it to RGB888.
 *
 * The function copies a cropped rectangle from a source RGB565 image and writes
 * it to a destination buffer in RGB888 format.
 *
 * @param src           Pointer to the input RGB565 image buffer.
 * @param src_width     Width of the source image in pixels.
 * @param src_height    Height of the source image in pixels.
 * @param dst           Pointer to the output RGB888 buffer.
 * @param crop_x        X coordinate of the top-left corner of the crop region.
 * @param crop_y        Y coordinate of the top-left corner of the crop region.
 * @param crop_width    Width of the crop region in pixels.
 * @param crop_height   Height of the crop region in pixels.
 */
void crop_rgb565_to_rgb888(const uint8_t *src,
                           int src_width,
                           int src_height,
                           uint8_t *dst,
                           int crop_x,
                           int crop_y,
                           int crop_width,
                           int crop_height);

/**
 * @brief Crop a region from an RGB888 image.
 *
 * The function copies a cropped rectangle from a source RGB888 image and writes
 * it to a destination buffer in RGB888 format.
 *
 * @param src           Pointer to the input RGB888 image buffer.
 * @param src_width     Width of the source image in pixels.
 * @param src_height    Height of the source image in pixels.
 * @param dst           Pointer to the output RGB888 buffer.
 * @param crop_x        X coordinate of the top-left corner of the crop region.
 * @param crop_y        Y coordinate of the top-left corner of the crop region.
 * @param crop_width    Width of the crop region in pixels.
 * @param crop_height   Height of the crop region in pixels.
 */
void crop_rgb888_to_rgb888(const uint8_t *src,
                           int src_width,
                           int src_height,
                           uint8_t *dst,
                           int crop_x,
                           int crop_y,
                           int crop_width,
                           int crop_height);
#ifdef __cplusplus
}
#endif
#endif /* IMAGE_PROCESSING_FUNC_H__ */
