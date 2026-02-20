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
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *---------------------------------------------------------------------------*/

#include <stdint.h>
#include "cmsis_compiler.h"
#include "image_processing_func.h"

/*
  Clamp a value to a specified range.

  This function ensures that the input value does not exceed the specified minimum
  and maximum bounds. If the value is less than the minimum, it returns the minimum.
  If it is greater than the maximum, it returns the maximum. Otherwise, it returns
  the value itself.

  \param[in] val  The value to clamp.
  \param[in] min  The minimum bound.
  \param[in] max  The maximum bound.
  \return         The clamped value.
*/
static inline int clamp(int val, int min, int max) {
  return (val < min) ? min : (val > max) ? max : val;
}

__WEAK void image_debayer(const uint8_t *raw,
                          uint8_t *rgb,
                          int width,
                          int height,
                          bayer_pattern_t pattern,
                          int swap_rb) {
  for (int y = 1; y < height - 1; ++y) {
    for (int x = 1; x < width - 1; ++x) {
      int idx = y * width + x;
      int r = 0, g = 0, b = 0;

      int is_even_row = (y & 1) == 0;
      int is_even_col = (x & 1) == 0;

      switch (pattern) {
        case BAYER_PATTERN_RGGB:
          if (is_even_row) {
            if (is_even_col) {
              // Red
              r = raw[idx];
              g = (raw[idx - 1] + raw[idx + 1] + raw[idx - width] + raw[idx + width]) >> 2;
              b = (raw[idx - width - 1] + raw[idx - width + 1] + raw[idx + width - 1] + raw[idx + width + 1]) >> 2;
            } else {
              // Green (on red row)
              g = raw[idx];
              r = (raw[idx - 1] + raw[idx + 1]) >> 1;
              b = (raw[idx - width] + raw[idx + width]) >> 1;
            }
          } else {
            if (is_even_col) {
              // Green (on blue row)
              g = raw[idx];
              r = (raw[idx - width] + raw[idx + width]) >> 1;
              b = (raw[idx - 1] + raw[idx + 1]) >> 1;
            } else {
              // Blue
              b = raw[idx];
              g = (raw[idx - 1] + raw[idx + 1] + raw[idx - width] + raw[idx + width]) >> 2;
              r = (raw[idx - width - 1] + raw[idx - width + 1] + raw[idx + width - 1] + raw[idx + width + 1]) >> 2;
            }
          }
          break;

        case BAYER_PATTERN_BGGR:
          if (is_even_row) {
            if (is_even_col) {
              // Blue
              b = raw[idx];
              g = (raw[idx - 1] + raw[idx + 1] + raw[idx - width] + raw[idx + width]) >> 2;
              r = (raw[idx - width - 1] + raw[idx - width + 1] + raw[idx + width - 1] + raw[idx + width + 1]) >> 2;
            } else {
              // Green (on blue row)
              g = raw[idx];
              b = (raw[idx - 1] + raw[idx + 1]) >> 1;
              r = (raw[idx - width] + raw[idx + width]) >> 1;
            }
          } else {
            if (is_even_col) {
              // Green (on red row)
              g = raw[idx];
              b = (raw[idx - width] + raw[idx + width]) >> 1;
              r = (raw[idx - 1] + raw[idx + 1]) >> 1;
            } else {
              // Red
              r = raw[idx];
              g = (raw[idx - 1] + raw[idx + 1] + raw[idx - width] + raw[idx + width]) >> 2;
              b = (raw[idx - width - 1] + raw[idx - width + 1] + raw[idx + width - 1] + raw[idx + width + 1]) >> 2;
            }
          }
          break;

        case BAYER_PATTERN_GRBG:
          if (is_even_row) {
            if (is_even_col) {
              // Green (on red row)
              g = raw[idx];
              r = (raw[idx - 1] + raw[idx + 1]) >> 1;
              b = (raw[idx - width] + raw[idx + width]) >> 1;
            } else {
              // Red
              r = raw[idx];
              g = (raw[idx - 1] + raw[idx + 1] + raw[idx - width] + raw[idx + width]) >> 2;
              b = (raw[idx - width - 1] + raw[idx - width + 1] + raw[idx + width - 1] + raw[idx + width + 1]) >> 2;
            }
          } else {
            if (is_even_col) {
              // Blue
              b = raw[idx];
              g = (raw[idx - 1] + raw[idx + 1] + raw[idx - width] + raw[idx + width]) >> 2;
              r = (raw[idx - width - 1] + raw[idx - width + 1] + raw[idx + width - 1] + raw[idx + width + 1]) >> 2;
            } else {
              // Green (on blue row)
              g = raw[idx];
              b = (raw[idx - 1] + raw[idx + 1]) >> 1;
              r = (raw[idx - width] + raw[idx + width]) >> 1;
            }
          }
          break;

        case BAYER_PATTERN_GBRG:
          if (is_even_row) {
            if (is_even_col) {
              // Green (on blue row)
              g = raw[idx];
              b = (raw[idx - 1] + raw[idx + 1]) >> 1;
              r = (raw[idx - width] + raw[idx + width]) >> 1;
            } else {
              // Blue
              b = raw[idx];
              g = (raw[idx - 1] + raw[idx + 1] + raw[idx - width] + raw[idx + width]) >> 2;
              r = (raw[idx - width - 1] + raw[idx - width + 1] + raw[idx + width - 1] + raw[idx + width + 1]) >> 2;
            }
          } else {
            if (is_even_col) {
              // Red
              r = raw[idx];
              g = (raw[idx - 1] + raw[idx + 1] + raw[idx - width] + raw[idx + width]) >> 2;
              b = (raw[idx - width - 1] + raw[idx - width + 1] + raw[idx + width - 1] + raw[idx + width + 1]) >> 2;
            } else {
              // Green (on red row)
              g = raw[idx];
              r = (raw[idx - 1] + raw[idx + 1]) >> 1;
              b = (raw[idx - width] + raw[idx + width]) >> 1;
            }
          }
          break;
      }

      int out_idx = (y * width + x) * 3;
      if (swap_rb == 0) {
        rgb[out_idx + 0] = (uint8_t)r;
        rgb[out_idx + 1] = (uint8_t)g;
        rgb[out_idx + 2] = (uint8_t)b;
      } else {
        rgb[out_idx + 0] = (uint8_t)b;
        rgb[out_idx + 1] = (uint8_t)g;
        rgb[out_idx + 2] = (uint8_t)r;
      }
    }
  }
}


__WEAK void crop_and_debayer(const uint8_t *src,
                             int src_width,
                             int src_height,
                             int src_crop_x,
                             int src_crop_y,
                             uint8_t *dst_rgb,
                             int dst_width,
                             int dst_height,
                             bayer_pattern_t pattern) {
  int offsets[2][2];
  switch (pattern) {
    case BAYER_PATTERN_BGGR: offsets[0][0] = 0; offsets[0][1] = 1; offsets[1][0] = 1; offsets[1][1] = 2; break;
    case BAYER_PATTERN_GBRG: offsets[0][0] = 1; offsets[0][1] = 0; offsets[1][0] = 2; offsets[1][1] = 1; break;
    case BAYER_PATTERN_GRBG: offsets[0][0] = 1; offsets[0][1] = 2; offsets[1][0] = 0; offsets[1][1] = 1; break;
    case BAYER_PATTERN_RGGB: offsets[0][0] = 2; offsets[0][1] = 1; offsets[1][0] = 1; offsets[1][1] = 0; break;
  }

  for (int dy = 0; dy < dst_height; ++dy) {
    int sy_fp = (dy * (src_height - 2 - src_crop_y * 2) << 8) / (dst_height - 1); // fixed-point
    int sy = sy_fp >> 8;
    int dy_frac = sy_fp & 0xFF;

    sy += src_crop_y;
    if (sy < 1) {
      sy = 1;
    }
    if (sy >= src_height - 2) {
      sy = src_height - 2;
    }

    for (int dx = 0; dx < dst_width; ++dx) {
      int sx_fp = (dx * (src_width - 2 - src_crop_x * 2) << 8) / (dst_width - 1); // fixed-point
      int sx = sx_fp >> 8;
      int dx_frac = sx_fp & 0xFF;

      sx += src_crop_x;
      if (sx < 1) {
        sx = 1;
      }
      if (sx >= src_width - 2) {
        sx = src_width - 2;
      }

      int row_parity = sy & 1;
      int col_parity = sx & 1;
      int offset = offsets[row_parity][col_parity];

      const uint8_t *p = src;
      int center = p[sy * src_width + sx];

      int r = 0;
      int g = 0;
      int b = 0;

      switch (offset) {
        case 0: // Blue
          b = center;
          g = (p[sy * src_width + sx - 1] + p[sy * src_width + sx + 1] +
               p[(sy - 1) * src_width + sx] + p[(sy + 1) * src_width + sx]) / 4;
          r = (p[(sy - 1) * src_width + sx - 1] + p[(sy - 1) * src_width + sx + 1] +
               p[(sy + 1) * src_width + sx - 1] + p[(sy + 1) * src_width + sx + 1]) / 4;
          break;

        case 1: // Green
          g = center;
          if ((row_parity == 0 && col_parity == 1) || (row_parity == 1 && col_parity == 0)) {
            r = (p[sy * src_width + sx - 1] + p[sy * src_width + sx + 1]) / 2;
            b = (p[(sy - 1) * src_width + sx] + p[(sy + 1) * src_width + sx]) / 2;
          } else {
            b = (p[sy * src_width + sx - 1] + p[sy * src_width + sx + 1]) / 2;
            r = (p[(sy - 1) * src_width + sx] + p[(sy + 1) * src_width + sx]) / 2;
          }
          break;

        case 2: // Red
          r = center;
          g = (p[sy * src_width + sx - 1] + p[sy * src_width + sx + 1] +
               p[(sy - 1) * src_width + sx] + p[(sy + 1) * src_width + sx]) / 4;
          b = (p[(sy - 1) * src_width + sx - 1] + p[(sy - 1) * src_width + sx + 1] +
               p[(sy + 1) * src_width + sx - 1] + p[(sy + 1) * src_width + sx + 1]) / 4;
          break;
      }

      int dst_idx = (dy * dst_width + dx) * 3;
      dst_rgb[dst_idx + 0] = (uint8_t)clamp(r, 0, 255);
      dst_rgb[dst_idx + 1] = (uint8_t)clamp(g, 0, 255);
      dst_rgb[dst_idx + 2] = (uint8_t)clamp(b, 0, 255);
    }
  }
}

#define FP_SHIFT 16
#define FP_ONE   (1 << FP_SHIFT)
#define FP_MASK  (FP_ONE - 1)

static void unpack_pixel(const uint8_t *buf, image_format_t format, int *r, int *g, int *b) {
  switch (format) {
    case IMAGE_FORMAT_GRAYSCALE:
      *r = *g = *b = buf[0];
      break;
    case IMAGE_FORMAT_RGB565: {
      uint16_t px = buf[0] | (buf[1] << 8);
      *r = ((px >> 11) & 0x1F) << 3;
      *g = ((px >> 5)  & 0x3F) << 2;
      *b = (px & 0x1F) << 3;
      break;
    }
    case IMAGE_FORMAT_RGB888:
      *r = buf[0];
      *g = buf[1];
      *b = buf[2];
      break;
  }
}

static void pack_pixel(uint8_t *buf, image_format_t format, int r, int g, int b) {
  switch (format) {
    case IMAGE_FORMAT_GRAYSCALE:
      buf[0] = (uint8_t)((r * 299 + g * 587 + b * 114) / 1000); // luminance
      break;
    case IMAGE_FORMAT_RGB565: {
      uint16_t px = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
      buf[0] = px & 0xFF;
      buf[1] = (px >> 8) & 0xFF;
      break;
    }
    case IMAGE_FORMAT_RGB888:
      buf[0] = (uint8_t)r;
      buf[1] = (uint8_t)g;
      buf[2] = (uint8_t)b;
      break;
  }
}

void crop_resize_rgb565_to_rgb888(
    const uint8_t *src,
    int src_width,
    int src_height,
    uint8_t *dst,
    int dst_width,
    int dst_height)
{
    int crop_size = src_height;                 // 720 for 1280x720
    int crop_x = (src_width - crop_size) / 2;   // center horizontally
    int crop_y = 0;

    int x_ratio = ((crop_size - 1) << FP_SHIFT) / (dst_width - 1);
    int y_ratio = ((crop_size - 1) << FP_SHIFT) / (dst_height - 1);

    for (int y = 0; y < dst_height; y++) {
        int src_y_fp = y * y_ratio;
        int sy = (src_y_fp >> FP_SHIFT) + crop_y;

        for (int x = 0; x < dst_width; x++) {
            int src_x_fp = x * x_ratio;
            int sx = (src_x_fp >> FP_SHIFT) + crop_x;

            int idx = (sy * src_width + sx) * 2;
            uint16_t pixel = src[idx] | (src[idx + 1] << 8);

            uint8_t r5 = (pixel >> 11) & 0x1F;
            uint8_t g6 = (pixel >> 5)  & 0x3F;
            uint8_t b5 = pixel & 0x1F;

            uint8_t *dst_pixel = &dst[(y * dst_width + x) * 3];

            dst_pixel[0] = (r5 << 3) | (r5 >> 2);
            dst_pixel[1] = (g6 << 2) | (g6 >> 4);
            dst_pixel[2] = (b5 << 3) | (b5 >> 2);
        }
    }
}

__WEAK void image_resize(const uint8_t *src,
                         int src_width,
                         int src_height,
                         uint8_t *dst,
                         int dst_width,
                         int dst_height,
                         image_format_t src_format,
                         image_format_t dst_format) {
  int src_bpp = (src_format == IMAGE_FORMAT_GRAYSCALE) ? 1 :
                (src_format == IMAGE_FORMAT_RGB565)    ? 2 : 3;
  int dst_bpp = (dst_format == IMAGE_FORMAT_GRAYSCALE) ? 1 :
                (dst_format == IMAGE_FORMAT_RGB565)    ? 2 : 3;

  int x_ratio = ((src_width - 1) << FP_SHIFT) / (dst_width - 1);
  int y_ratio = ((src_height - 1) << FP_SHIFT) / (dst_height - 1);

  for (int y = 0; y < dst_height; ++y) {
    int src_y_fp = y * y_ratio;
    int y0 = src_y_fp >> FP_SHIFT;
    int y1 = (y0 < src_height - 1) ? y0 + 1 : y0;
    int wy = src_y_fp & FP_MASK;

    for (int x = 0; x < dst_width; ++x) {
      int src_x_fp = x * x_ratio;
      int x0 = src_x_fp >> FP_SHIFT;
      int x1 = (x0 < src_width - 1) ? x0 + 1 : x0;
      int wx = src_x_fp & FP_MASK;

      int r00, g00, b00;
      int r01, g01, b01;
      int r10, g10, b10;
      int r11, g11, b11;

      const uint8_t *p00 = &src[(y0 * src_width + x0) * src_bpp];
      const uint8_t *p01 = &src[(y0 * src_width + x1) * src_bpp];
      const uint8_t *p10 = &src[(y1 * src_width + x0) * src_bpp];
      const uint8_t *p11 = &src[(y1 * src_width + x1) * src_bpp];

      unpack_pixel(p00, src_format, &r00, &g00, &b00);
      unpack_pixel(p01, src_format, &r01, &g01, &b01);
      unpack_pixel(p10, src_format, &r10, &g10, &b10);
      unpack_pixel(p11, src_format, &r11, &g11, &b11);

      // Interpolate each channel
      int r_top = ((FP_ONE - wx) * r00 + wx * r01) >> FP_SHIFT;
      int r_bot = ((FP_ONE - wx) * r10 + wx * r11) >> FP_SHIFT;
      int r = ((FP_ONE - wy) * r_top + wy * r_bot) >> FP_SHIFT;

      int g_top = ((FP_ONE - wx) * g00 + wx * g01) >> FP_SHIFT;
      int g_bot = ((FP_ONE - wx) * g10 + wx * g11) >> FP_SHIFT;
      int g = ((FP_ONE - wy) * g_top + wy * g_bot) >> FP_SHIFT;

      int b_top = ((FP_ONE - wx) * b00 + wx * b01) >> FP_SHIFT;
      int b_bot = ((FP_ONE - wx) * b10 + wx * b11) >> FP_SHIFT;
      int b = ((FP_ONE - wy) * b_top + wy * b_bot) >> FP_SHIFT;

      uint8_t *dst_pixel = &dst[(y * dst_width + x) * dst_bpp];
      pack_pixel(dst_pixel, dst_format, r, g, b);
    }
  }
}

__WEAK void image_copy_to_framebuffer(const uint8_t *src,
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
                                      int swap_rb) {
  int bpp;

  switch (format) {
    case IMAGE_FORMAT_GRAYSCALE:
      bpp = 1;
      break;
    case IMAGE_FORMAT_RGB565:
      bpp = 2;
      break;
    case IMAGE_FORMAT_RGB888:
      bpp = 3;
      break;
    default:
      return; // unsupported format
  }

  for (int y = 0; y < src_height; ++y) {
    int dst_y = y + y_offset;
    if (dst_y < 0 || dst_y >= dst_height)
      continue;

    for (int x = 0; x < src_width; ++x) {
      int dst_x = x + x_offset;
      if (dst_x < 0 || dst_x >= dst_width)
        continue;

      int src_y = flip_vertical ? (src_height - 1 - y) : y;
      int src_x = flip_horizontal ? (src_width - 1 - x) : x;
      int src_idx = (src_y * src_width + src_x) * bpp;
      int dst_idx = (dst_y * dst_width + dst_x) * bpp;

      if ((format == IMAGE_FORMAT_RGB888) && (swap_rb != 0)) {
        dst[dst_idx + 0] = src[src_idx + 2];
        dst[dst_idx + 1] = src[src_idx + 1];
        dst[dst_idx + 2] = src[src_idx + 0];
      } else {
        for (int i = 0; i < bpp; ++i) {
          dst[dst_idx + i] = src[src_idx + i];
        }
      }
    }
  }
}

__WEAK void convert_rgb565_to_rgb888(const uint8_t *src,
                                     uint8_t *dst,
                                     int width,
                                     int height) {
  for (int i = 0; i < width * height; ++i) {
    uint16_t pixel = src[2 * i] | (src[2 * i + 1] << 8);

    /* Extract RGB components from RGB565 */
    uint8_t r5 = (pixel >> 11) & 0x1F;
    uint8_t g6 = (pixel >> 5) & 0x3F;
    uint8_t b5 = pixel & 0x1F;

    /* Convert to 8-bit RGB888 */
    uint8_t r8 = (r5 << 3) | (r5 >> 2);  // replicate upper bits
    uint8_t g8 = (g6 << 2) | (g6 >> 4);
    uint8_t b8 = (b5 << 3) | (b5 >> 2);

    dst[3 * i + 0] = r8;
    dst[3 * i + 1] = g8;
    dst[3 * i + 2] = b8;
  }
}

__WEAK void crop_rgb565_to_rgb888(const uint8_t *src,
                                  int src_width,
                                  int src_height,
                                  uint8_t *dst,
                                  int crop_x,
                                  int crop_y,
                                  int crop_width,
                                  int crop_height) {
  for (int y = 0; y < crop_height; ++y) {
    int src_y = crop_y + y;
    if (src_y >= src_height) break;

    for (int x = 0; x < crop_width; ++x) {
      int src_x = crop_x + x;
      if (src_x >= src_width) break;

      int src_idx = (src_y * src_width + src_x) * 2;
      uint16_t pixel = src[src_idx] | (src[src_idx + 1] << 8);

      // Extract RGB565 components
      uint8_t r5 = (pixel >> 11) & 0x1F;
      uint8_t g6 = (pixel >> 5)  & 0x3F;
      uint8_t b5 = pixel & 0x1F;

      // Convert to RGB888
      uint8_t r8 = (r5 << 3) | (r5 >> 2);
      uint8_t g8 = (g6 << 2) | (g6 >> 4);
      uint8_t b8 = (b5 << 3) | (b5 >> 2);

      int dst_idx = (y * crop_width + x) * 3;
      dst[dst_idx + 0] = r8;
      dst[dst_idx + 1] = g8;
      dst[dst_idx + 2] = b8;
    }
  }
}

__WEAK void crop_rgb888_to_rgb888(const uint8_t *src,
                                  int src_width,
                                  int src_height,
                                  uint8_t *dst,
                                  int crop_x,
                                  int crop_y,
                                  int crop_width,
                                  int crop_height)
{
  const int bpp = 3; // RGB888 = 3 bytes per pixel

  for (int y = 0; y < crop_height; ++y) {
    int src_y = crop_y + y;
    if (src_y >= src_height) break; // Prevent out-of-bounds

    const uint8_t *src_row = src + (src_y * src_width + crop_x) * bpp;
    uint8_t *dst_row = dst + (y * crop_width) * bpp;

    for (int x = 0; x < crop_width; ++x) {
      int src_x = x;
      if ((crop_x + src_x) >= src_width) break;

      const uint8_t *src_pixel = src_row + src_x * bpp;
      uint8_t *dst_pixel = dst_row + x * bpp;

      dst_pixel[0] = src_pixel[0]; // R
      dst_pixel[1] = src_pixel[1]; // G
      dst_pixel[2] = src_pixel[2]; // B
    }
  }
}
