/*
 * SPDX-FileCopyrightText: Copyright 2022, 2024 Arm Limited and/or its
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

#ifndef CONFIG_APP__
#define CONFIG_APP__

//-------- <<< Use Configuration Wizard in Context Menu >>> --------------------

// <h>Buffer Configuration
// ===================================

//  <o>Activation Buffer Size
//  <i> Define the size in bytes of the activation buffer.
//  <i> Default: 1048576 (1MB)
#ifndef ACTIVATION_BUF_SZ
#define ACTIVATION_BUF_SZ           532480
#endif

//  <s>Activation Buffer Section Name
//  <i> Define the name of the activation buffer section
//  <i> Default: ".bss.activation_buf"
#ifndef ACTIVATION_BUF_SECTION
#define ACTIVATION_BUF_SECTION      ".bss.activation_buf"
#endif

//  <o>Activation Buffer Alignment
//  <i> Define the activation buffer alignment in bytes
//  <i> Default: 16
#ifndef ACTIVATION_BUF_ALIGNMENT
#define ACTIVATION_BUF_ALIGNMENT    16
#endif

//  <s>NN Model Buffer Section Name
//  <i> Define the name of the NN model buffer section
//  <i> Default: "nn_model"
#ifndef NN_MODEL_BUF_SECTION
#define NN_MODEL_BUF_SECTION        "nn_model"
#endif
//  <o>NN Model Buffer Alignment
//  <i> Define the NN model buffer alignment in bytes
//  <i> Default: 16
#ifndef NN_MODEL_BUF_ALIGNMENT
#define NN_MODEL_BUF_ALIGNMENT      16
#endif

// </h>

#endif /* CONFIG_APP__ */
