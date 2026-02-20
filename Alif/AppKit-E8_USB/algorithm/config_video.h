/*
 * SPDX-FileCopyrightText: Copyright 2025 Arm Limited and/or its
 * affiliates <open-source-office@arm.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CONFIG_VIDEO_H__
#define CONFIG_VIDEO_H__

//-------- <<< Use Configuration Wizard in Context Menu >>> --------------------

// <h>Camera Configuration
// =======================

//  <o>Camera Frame Width
//  <i> Define the camera frame width.
//  <i> Common camera frame widths: 320, 640, 1024, 1280.
//  <i> Default: 640
#ifndef CAMERA_FRAME_WIDTH
#define CAMERA_FRAME_WIDTH          1280
#endif

//  <o>Camera Frame Height
//  <i> Define the camera frame height.
//  <i> Common camera frame heights: 240, 480, 600, 720.
//  <i> Default: 480
#ifndef CAMERA_FRAME_HEIGHT
#define CAMERA_FRAME_HEIGHT         720
#endif

//  <o>Frame Type <0=>RAW8 <1=>RGB565 <2=>RGB888
//  <i> Define whether camera frame is raw or RGB.
//  <i> Default: 1
#ifndef CAMERA_FRAME_TYPE
#define CAMERA_FRAME_TYPE           1
#endif

//  <o>Frame Bayer Pattern <0=>RGGB <1=>BGGR <2=>GRBG <3=>GBRG
//  <i> Define the raw camera frame Bayer pattern.
//  <i> Default: 3
#ifndef CAMERA_FRAME_BAYER
#define CAMERA_FRAME_BAYER          3
#endif

//  <s>Frame Buffer Section Name
//  <i> Define the name of the camera frame buffer section.
//  <i> Default: ".bss.camera_frame_buf"
#ifndef CAMERA_FRAME_BUF_SECTION
#define CAMERA_FRAME_BUF_SECTION    ".bss.camera_frame_buf"
#endif

//  <o>Frame Buffer Alignment
//  <i> Define the camera frame buffer alignment in bytes.
//  <i> Default: 32
#ifndef CAMERA_FRAME_BUF_ALIGNMENT
#define CAMERA_FRAME_BUF_ALIGNMENT  32
#endif

//  <o>RGB Image Width
//  <i> Define the RGB image width.
//  <i> Default: 384
#ifndef RGB_IMAGE_WIDTH
#define RGB_IMAGE_WIDTH             384
#endif

//  <o>RGB Image Height
//  <i> Define the RGB image height.
//  <i> Default: 384
#ifndef RGB_IMAGE_HEIGHT
#define RGB_IMAGE_HEIGHT            384
#endif

//  <s>RGB Image Buffer Section Name
//  <i> Define the name of the RGB image buffer section.
//  <i> Default: ".bss.rgb_image_buf"
#ifndef RGB_IMAGE_BUF_SECTION
#define RGB_IMAGE_BUF_SECTION       ".bss.rgb_image_buf"
#endif

//  <o>RGB Image Buffer Alignment
//  <i>Define the RGB image buffer alignment in bytes.
//  <i>Default: 4
#ifndef RGB_IMAGE_BUF_ALIGNMENT
#define RGB_IMAGE_BUF_ALIGNMENT     4
#endif

// </h>

// <h>Display Configuration
// ========================

//  <o>Display Frame Width
//  <i> Defines the display frame width.
//  <i> Common display frame widths: 480, 800, 1024, 1280.
//  <i> Default: 800
#ifndef DISPLAY_FRAME_WIDTH
#define DISPLAY_FRAME_WIDTH         480
#endif

//  <o>Display Frame Height
//  <i> Defines the display frame height.
//  <i> Common display frame heights: 320, 480, 600, 800.
//  <i> Default: 480
#ifndef DISPLAY_FRAME_HEIGHT
#define DISPLAY_FRAME_HEIGHT        800
#endif

//  <s>Frame Buffer Section Name
//  <i> Define the name of the display frame buffer section
//  <i> Default: ".bss.lcd_frame_buf"
#ifndef DISPLAY_FRAME_BUF_SECTION
#define DISPLAY_FRAME_BUF_SECTION   ".bss.lcd_frame_buf"
#endif

//  <o>Frame Buffer Alignment
//  <i> Define the display frame buffer alignment in bytes
//  <i> Default: 32
#ifndef DISPLAY_FRAME_BUF_ALIGNMENT
#define DISPLAY_FRAME_BUF_ALIGNMENT 32
#endif

//  <o>Display Flip Horizontal <0=>Disable <1=>Enable
//  <i> Enable to mirror display content left-right.
//  <i> Default: 1
#ifndef DISPLAY_FLIP_HORIZONTAL
#define DISPLAY_FLIP_HORIZONTAL    1
#endif

//  <o>Display Flip Vertical <0=>Disable <1=>Enable
//  <i> Enable to mirror display content top-bottom.
//  <i> Default: 1
#ifndef DISPLAY_FLIP_VERTICAL
#define DISPLAY_FLIP_VERTICAL      1
#endif

//  <o>Display Swap R/B <0=>Disable <1=>Enable
//  <i> Enable when panel output appears in BGR instead of RGB.
//  <i> Default: 1
#ifndef DISPLAY_SWAP_RB
#define DISPLAY_SWAP_RB            1
#endif

// </h>

#endif /* CONFIG_VIDEO_H__ */
