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

#ifndef CONFIG_ML_MODEL_H__
#define CONFIG_ML_MODEL_H__

// ML Image Width
// Define the width of the image to be used for ML inference.
// Default: 224
#ifndef ML_IMAGE_WIDTH
#define ML_IMAGE_WIDTH              224
#endif
// ML Image Height
// Define the height of the image to be used for ML inference.
// Default: 224
#ifndef ML_IMAGE_HEIGHT
#define ML_IMAGE_HEIGHT             224
#endif
// ML Image Buffer Section Name
// Define the name of the section for the ML image buffer
// Default: ".bss.ml_image_buf"
#ifndef ML_IMAGE_BUF_SECTION
#define ML_IMAGE_BUF_SECTION        ".bss.ml_image_buf"
#endif
// ML Image Buffer Alignment
// Define the alignment in bytes for the ML image buffer
// Default: 4
#ifndef ML_IMAGE_BUF_ALIGNMENT
#define ML_IMAGE_BUF_ALIGNMENT      4
#endif

#endif /* CONFIG_ML_MODEL_H__ */