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

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include <utility>

#include "cmsis_vstream.h"
#include "config_video.h"          /* DISPLAY_IMAGE_SIZE, DISPLAY_FRAME_BUF_ATTRIBUTE */
#include "sds_algorithm_config.h"
#include "sds_algorithm.h"
#include "app_setup.h"
#include "arm_executor_runner.h"
#include "image_processing_func.h"
#include "model_pte.h"
#include "profiler.h"
#ifdef USE_SEGGER_SYSVIEW
#include "SEGGER_SYSVIEW.h"
#include "sysview_markers.h"
#endif

#include <executorch/extension/data_loader/buffer_data_loader.h>
#include <executorch/runtime/executor/program.h>

using executorch::extension::BufferDataLoader;
using executorch::runtime::Program;
using executorch::runtime::Result;

/* ============================================================================
 * File-scope state  (persists across InitAlgorithm / ExecuteAlgorithm calls)
 * ============================================================================
 */

/* Display frame buffer (RGB888) */
static uint8_t LCD_Frame[DISPLAY_IMAGE_SIZE] DISPLAY_FRAME_BUF_ATTRIBUTE;

/* Runner state – must survive across ExecuteAlgorithm() calls */
static RunnerContext *ctx = nullptr;

/* In-place storage for BufferDataLoader and Result<Program> (no heap) */
static uint8_t loader_storage[sizeof(BufferDataLoader)]
    __attribute__((aligned(alignof(BufferDataLoader))));
static uint8_t program_result_storage[sizeof(Result<Program>)]
    __attribute__((aligned(alignof(Result<Program>))));

/* Pointers into the above storage */
static BufferDataLoader *loader_ptr  = nullptr;
static Program          *program_ptr = nullptr;

/* Reference to the underlying CMSIS vStream VideoOut driver */
extern vStreamDriver_t Driver_vStreamVideoOut;
#define vStream_VideoOut  (&Driver_vStreamVideoOut)

/* Video Out Stream Event Callback */
static void VideoOut_Event_Callback(uint32_t event) {
    (void)event;
}

/* ============================================================================
 * InitAlgorithm
 * ============================================================================
 */

/**
  \fn           int32_t InitAlgorithm (void)
  \brief        Initialize algorithm under test.
  \return       0 on success; -1 on error
*/
int32_t InitAlgorithm(void) {

    /* ---- Video Output Stream ---- */
    if (vStream_VideoOut->Initialize(VideoOut_Event_Callback) != VSTREAM_OK) {
        printf("Failed to initialise video output driver\n");
        return -1;
    }

    if (vStream_VideoOut->SetBuf(LCD_Frame, sizeof(LCD_Frame), DISPLAY_IMAGE_SIZE) != VSTREAM_OK) {
        printf("Failed to set buffer for video output\n");
        return -1;
    }

    /* ---- Model Loading ---- */
    size_t pte_size = sizeof(model_pte);

    /* Construct BufferDataLoader in-place (no heap allocation) */
    loader_ptr = new (loader_storage) BufferDataLoader(model_pte, pte_size);

    /* Load the ExecuTorch program in-place */
    auto *program_result = new (program_result_storage)
        Result<Program>(Program::load(loader_ptr));

    if (!program_result->ok()) {
        printf("Program loading failed: 0x%" PRIx32 "\n",
               (uint32_t)program_result->error());
        return -1;
    }

    program_ptr = &program_result->get();

    /* ---- Runner Init (loads model method into RunnerContext) ---- */
    ctx = runner_context_instance();
    std::vector<std::pair<char *, size_t>> input_buffers; /* empty on init */
    runner_init(*ctx, input_buffers, pte_size, program_ptr);

    return 0;
}

/* ============================================================================
 * ExecuteAlgorithm
 * ============================================================================
 */

/**
  \fn           int32_t ExecuteAlgorithm (uint8_t *in_buf, uint32_t in_num, uint8_t *out_buf, uint32_t out_num)
  \brief        Execute algorithm under test.
  \param[in]    in_buf   pointer to input frame buffer (RGB888, HWC, 224x224x3)
  \param[in]    in_num   number of bytes in input buffer
  \param[out]   out_buf  pointer to output buffer (receives runner_output_label_t)
  \param[in]    out_num  maximum bytes available in output buffer
  \return       0 on success; -1 on error
*/
int32_t ExecuteAlgorithm(uint8_t *in_buf, uint32_t in_num,
                         uint8_t *out_buf, uint32_t out_num) {

    vStreamStatus_t v_status;
    uint8_t        *outFrame;

    /* Clear output buffer */
    memset(out_buf, 0, out_num);

    /* ---- Pre-processing: HWC→CHW + ImageNet normalisation ---- */
#ifdef USE_SEGGER_SYSVIEW
    SEGGER_SYSVIEW_MarkStart(SYSVIEW_MARKER_PRE_PROCESS);
#endif
#if ENABLE_TIME_PROFILING
    uint32_t pre_process_time = profiler_start();
#endif

    preprocess(in_buf);

#ifdef USE_SEGGER_SYSVIEW
    SEGGER_SYSVIEW_MarkStop(SYSVIEW_MARKER_PRE_PROCESS);
#endif
#if ENABLE_TIME_PROFILING
    pre_process_time = profiler_stop(pre_process_time);
    printf("Pre Processing time: %3.3f ms.\n",
           profiler_cycles_to_ms(pre_process_time, CPU_FREQ_HZ));
#endif

    /* ---- Inference ---- */
    if (!run_inference(*ctx)) {
        printf("Inference failed.\n");
        return -1;
    }

    /* ---- Post-processing: decode output tensor into output_label ---- */
#ifdef USE_SEGGER_SYSVIEW
    SEGGER_SYSVIEW_MarkStart(SYSVIEW_MARKER_POST_PROCESS);
#endif
#if ENABLE_TIME_PROFILING
    uint32_t post_process_time = profiler_start();
#endif

    postprocess(*ctx, in_buf, out_buf, out_num);

#ifdef USE_SEGGER_SYSVIEW
    SEGGER_SYSVIEW_MarkStop(SYSVIEW_MARKER_POST_PROCESS);
#endif
#if ENABLE_TIME_PROFILING
    post_process_time = profiler_stop(post_process_time);
    printf("Post Process time: %3.3f ms.\n",
           profiler_cycles_to_ms(post_process_time, CPU_FREQ_HZ));
#endif

    /* ---- Display: copy ML frame to LCD framebuffer ---- */
#ifdef USE_SEGGER_SYSVIEW
    SEGGER_SYSVIEW_MarkStart(SYSVIEW_MARKER_DISPLAY);
#endif
#if ENABLE_TIME_PROFILING
    uint32_t display_time = profiler_start();
#endif

    /* Wait for previous video output frame to finish */
    do {
        v_status = vStream_VideoOut->GetStatus();
    } while (v_status.active == 1U);

    outFrame = (uint8_t *)vStream_VideoOut->GetBlock();
    if (outFrame == NULL) {
        printf("Failed to get video output frame\n");
        return -1;
    }

    image_copy_to_framebuffer(
        in_buf,
        IMAGE_WIDTH,  IMAGE_HEIGHT,
        outFrame,
        DISPLAY_FRAME_WIDTH, DISPLAY_FRAME_HEIGHT,
        (DISPLAY_FRAME_WIDTH  - IMAGE_WIDTH)  / 2,
        (DISPLAY_FRAME_HEIGHT - IMAGE_HEIGHT) / 2,
        IMAGE_FORMAT_RGB888,
        DISPLAY_FLIP_HORIZONTAL,
        DISPLAY_FLIP_VERTICAL,
        DISPLAY_SWAP_RB);

#ifdef USE_SEGGER_SYSVIEW
    SEGGER_SYSVIEW_MarkStop(SYSVIEW_MARKER_DISPLAY);
#endif
#if ENABLE_TIME_PROFILING
    display_time = profiler_stop(display_time);
    printf("Display time: %3.3f ms.\n",
           profiler_cycles_to_ms(display_time, CPU_FREQ_HZ));
#endif

    if (vStream_VideoOut->ReleaseBlock() != VSTREAM_OK) {
        printf("Failed to release video output frame\n");
    }

    if (vStream_VideoOut->Start(VSTREAM_MODE_SINGLE) != VSTREAM_OK) {
        printf("Failed to start video output\n");
    }

    return 0;
}
