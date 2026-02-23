/**
 * \file arm_executor_runner.h
 * \brief C++ interface for the ExecuTorch ARM model runner.
 */

#ifndef ARM_EXECUTOR_RUNNER_H
#define ARM_EXECUTOR_RUNNER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <vector>
#include <utility>

#include <executorch/extension/data_loader/buffer_data_loader.h>
#include <executorch/runtime/executor/program.h>

/* ============================================================================
 * Constants
 * ============================================================================
 */

#define IMAGE_HEIGHT                 224
#define IMAGE_WIDTH                  224
#define IMAGE_CHANNELS               3
#define MAX_LABEL_NAME_LENGTH        100
#define OUTPUT_STRING_SIZE           100

/* ============================================================================
 * Classification Result
 * ============================================================================
 */

/**
 * \brief Classification result from one inference run.
 *        Populated by print_outputs(). Copy into out_buf in ExecuteAlgorithm().
 */
typedef struct {
    char  label_name[MAX_LABEL_NAME_LENGTH]; /**< Null-terminated predicted class name */
    float confidence;                        /**< Confidence score */
} runner_output_label_t;

/* ============================================================================
 * Forward Declaration of RunnerContext
 * ============================================================================
 */
struct RunnerContext;

/**
 * \brief Get a process-lifetime RunnerContext owned by arm_executor_runner.cc.
 *        Use this when only an opaque pointer is needed in other translation units.
 *
 * \return Pointer to an internal static RunnerContext.
 */
RunnerContext* runner_context_instance(void);

/* ============================================================================
 * Pipeline Function Declarations
 * ============================================================================
 */

/**
 * \brief Initialise a RunnerContext and load the model method.
 *        Call once inside InitAlgorithm(), before the first run_inference().
 *
 * \param[in,out] ctx           RunnerContext to initialise.
 * \param[in]     input_buffers Pass an empty vector on init.
 * \param[in]     pte_size      Byte size of the model PTE blob.
 * \param[in]     program       Pointer to an already-loaded Program instance.
 */
void runner_init(RunnerContext                              &ctx,
                 std::vector<std::pair<char *, size_t>>     input_buffers,
                 size_t                                     pte_size,
                 executorch::runtime::Program              *program);

/**
 * \brief Convert one RGB888 HWC frame into the model's float input tensor.
 *        Transposes HWC→CHW and applies ImageNet normalisation.
 *        Must be called before run_inference() each frame.
 *
 * \param[in] image  RGB888 HWC source image (224 x 224 x 3 bytes).
 */
void preprocess(const uint8_t *image);

/**
 * \brief Execute one inference cycle on the pre-processed input tensor.
 *
 * \param[in,out] ctx  Initialised RunnerContext (from runner_init()).
 * \return true on success, false on failure.
 */
bool run_inference(RunnerContext &ctx);

/**
 * \brief Full post-processing: decode outputs, copy result, draw label on frame.
 *
 * \param[in]     ctx      RunnerContext after a successful run_inference().
 * \param[in]     img_buf  RGB888 frame buffer to draw the label onto.
 * \param[out]    out_buf  Caller buffer to receive runner_output_label_t result.
 * \param[in]     out_num  Byte size of out_buf.
 */
void postprocess(RunnerContext &ctx, uint8_t *img_buf,
                 uint8_t *out_buf, uint32_t out_num);

#endif /* ARM_EXECUTOR_RUNNER_H */