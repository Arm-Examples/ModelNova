/**
 * \file executorch_runner.cpp
 * \brief ExecuTorch model runner implementation for image classification
 *        Handles model loading, inference execution, and result visualization
 */

#include <errno.h>
#include <executorch/extension/data_loader/buffer_data_loader.h>
#include <executorch/extension/runner_util/inputs.h>
#include <executorch/runtime/core/memory_allocator.h>
#include <executorch/runtime/executor/program.h>
#include <executorch/runtime/platform/log.h>
#include <executorch/runtime/platform/platform.h>
#include <executorch/runtime/platform/runtime.h>
#include <math.h>
#include <stdio.h>
#include <memory>
#include <vector>
#include "RTE_Components.h"
#include "cmsis_vstream.h"
#include "config_video.h"
#include "image_processing_func.h"
#include CMSIS_device_header
#include "arm_memory_allocator.h"
#include "profiler.h"
#include "arm_executor_runner.h"  /* runner_output_label_t, RunnerContext (shared with sds_algorithm_user.cpp) */

// AC6 (armclang) doesn't have unistd.h in bare-metal mode
// Use stdlib exit() instead of _exit() for AC6
#if defined(__ARMCC_VERSION)
  // AC6 bare-metal: use exit() from stdlib.h
  #define _exit(code) exit(code)
#else
  #include <unistd.h>
#endif

using executorch::aten::ScalarType;
using executorch::aten::Tensor;
using executorch::extension::BufferDataLoader;
using executorch::runtime::Error;
using executorch::runtime::EValue;
using executorch::runtime::HierarchicalAllocator;
using executorch::runtime::MemoryAllocator;
using executorch::runtime::MemoryManager;
using executorch::runtime::Method;
using executorch::runtime::MethodMeta;
using executorch::runtime::Program;
using executorch::runtime::Result;
using executorch::runtime::Span;
using executorch::runtime::Tag;
using executorch::runtime::TensorInfo;

/* ============================================================================
 * Macros and Definitions
 * ============================================================================
 */

#define VEHICLE_MODEL            0
#define BANANA_RIPENESS_MODEL    1
#define TOOL_MODEL               2
#define RPS_MODEL                3
#define NUM_CLASSES              4
#define FONT_WIDTH               8
#define FONT_HEIGHT              8
#define FONT_SCALE               2
#define TEXT_BOTTOM_MARGIN       8
#define OUTPUT_STRING_SIZE       100
#define MAX_LABEL_NAME_LENGTH    100
#define ASCII_CHAR_COUNT         128
#define BYTES_PER_PIXEL_RGB888   3
#define FONT_MSB_MASK            0x80
#define PERCENT_SCALE            100.0f

#define COLOR_RESET              "\033[0m"
#define COLOR_RED                "\033[31m"
#define COLOR_GREEN              "\033[32m"
#define COLOR_YELLOW             "\033[33m"
#define COLOR_BLUE               "\033[34m"


#define vStream_VideoOut         (&Driver_vStreamVideoOut)

#if !defined(ET_ARM_BAREMETAL_METHOD_ALLOCATOR_POOL_SIZE)
#define ET_ARM_BAREMETAL_METHOD_ALLOCATOR_POOL_SIZE (60 * 1024 * 1024)
#endif

extern vStreamDriver_t Driver_vStreamVideoOut;

/**
* Implementation of the et_pal_<funcs>()
*
* This functions are hardware adaption type of functions for things like
* time/logging/memory allocation that could call your RTOS or need to to
* be implemnted in some way.
*/

void et_pal_init(void) {
  // PMU initialization - using alternative approach if CMSIS PMU not available
#if defined(__ARM_FEATURE_PMU_DWT)
  // Enable cycle counter using DWT (Data Watchpoint and Trace)
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
#elif defined(__PMU_PRESENT) 
  // Use ARM PMU if available
  ARM_PMU_Enable();
  DCB->DEMCR |= DCB_DEMCR_TRCENA_Msk; // Trace enable
  ARM_PMU_CYCCNT_Reset();
  ARM_PMU_CNTR_Enable(PMU_CNTENSET_CCNTR_ENABLE_Msk);
#else
  // Fallback: basic initialization without PMU
  // Performance timing will not be accurate
#endif
}


ET_NORETURN void et_pal_abort(void) {
#if !defined(SEMIHOSTING)
  __builtin_trap();
#else
  _exit(-1);
#endif
}

et_timestamp_t et_pal_current_ticks(void) {
#if defined(__ARM_FEATURE_PMU_DWT)
  // Use DWT cycle counter if available
  return DWT->CYCCNT;
#elif defined(ARM_PMU_Get_CCNTR)
  // Use ARM PMU if available
  return ARM_PMU_Get_CCNTR();
#else
  // Fallback: return a basic counter (not cycle accurate)
  static uint32_t tick_counter = 0;
  return ++tick_counter;
#endif
}

et_tick_ratio_t et_pal_ticks_to_ns_multiplier(void) {
  // Since we don't know the CPU freq for your target and justs cycles in the
  // FVP for et_pal_current_ticks() we return a conversion ratio of 1
  return {1, 1};
}

/* ============================================================================
 * Type Definitions
 * ============================================================================
 */

/* output_label_t is defined as runner_output_label_t in arm_executor_runner.h */
typedef runner_output_label_t output_label_t;

/* ============================================================================
 * External Variables
 * ============================================================================
 */

/** \brief Video output stream driver */
extern vStreamDriver_t Driver_vStreamVideoOut;

/* ============================================================================
 * Global Variables
 * ============================================================================
 */
char output_string[OUTPUT_STRING_SIZE];

int model_config = RPS_MODEL;

float conf_score = 0.0f;

int conf_int;

char label_name[MAX_LABEL_NAME_LENGTH] = {0};

bool classify_object = false;

output_label_t output_label;

constexpr int H = IMAGE_HEIGHT;

constexpr int W = IMAGE_WIDTH;

constexpr int C = IMAGE_CHANNELS;

static float input_tensor_data[1 * C * H * W];

constexpr float mean[3] = {0.485f, 0.456f, 0.406f};

constexpr float stdv[3] = {0.229f, 0.224f, 0.225f};

const size_t method_allocation_pool_size =
    ET_ARM_BAREMETAL_METHOD_ALLOCATOR_POOL_SIZE;

unsigned char __attribute__((
    section("input_data_sec"),
    aligned(16))) method_allocation_pool[method_allocation_pool_size];

const int num_inferences = 1;

const size_t temp_allocation_pool_size =
    ET_ARM_BAREMETAL_SCRATCH_TEMP_ALLOCATOR_POOL_SIZE;
unsigned char __attribute__((
    section(".bss.NoInit.activation_buf_sram"),
    aligned(16))) temp_allocation_pool[temp_allocation_pool_size];

#if ENABLE_TIME_PROFILING
/** \brief Loading time in cycles */
uint32_t loading_time = 0;

/** \brief Preprocessing time in cycles */
uint32_t pre_process_time = 0;

/** \brief Inference time in cycles */
uint32_t inference_time = 0;

/** \brief Printing time in cycles */
uint32_t printing_time = 0;

/** \brief Postprocessing time in cycles */
uint32_t post_process_time = 0;

/** \brief Buffer load time in cycles */
uint32_t buffer_load_time = 0;

/** \brief Display time in cycles */
uint32_t display_time = 0;
#endif

/* ============================================================================
 * Static Constants
 * ============================================================================
 */
static const char* VEHICLE_CLASS_NAMES[] = {"UNKNOWN", "BUS", "CAR", "TRUCK"};
static const char* BANANA_CLASS_NAMES[] = {"Overripe", "Ripe", "Rotten",
                                           "Unripe"};
static const char* TOOL_CLASS_NAMES[] = {"Bolt", "Hammer", "Nail", "Nut"};
static const char* RPS_CLASS_NAMES[] = {"PAPER", "ROCK", "SCISSORS", "UNKNOWN"};

/* Traditional array initialization (no designated initializers) */
static const uint8_t font_8x8[ASCII_CHAR_COUNT][8] = {
    /* Initialize first 32 entries (control characters) to blank */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 0 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 1 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 2 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 3 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 4 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 5 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 6 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 7 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 8 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 9 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 10 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 11 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 12 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 13 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 14 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 15 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 16 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 17 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 18 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 19 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 20 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 21 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 22 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 23 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 24 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 25 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 26 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 27 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 28 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 29 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 30 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 31 */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 32 ' ' (space) */
    {0x18, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18, 0x00}, /* 33 '!' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 34 '"' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 35 '#' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 36 '$' */
    {0x62, 0x66, 0x0C, 0x18, 0x30, 0x66, 0x46, 0x00}, /* 37 '%' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 38 '&' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 39 ''' */
    {0x0C, 0x18, 0x30, 0x30, 0x30, 0x18, 0x0C, 0x00}, /* 40 '(' */
    {0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x18, 0x30, 0x00}, /* 41 ')' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 42 '*' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 43 '+' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 44 ',' */
    {0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00}, /* 45 '-' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00}, /* 46 '.' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 47 '/' */
    {0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00}, /* 48 '0' */
    {0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00}, /* 49 '1' */
    {0x3C, 0x66, 0x06, 0x0C, 0x18, 0x30, 0x7E, 0x00}, /* 50 '2' */
    {0x3C, 0x66, 0x06, 0x1C, 0x06, 0x66, 0x3C, 0x00}, /* 51 '3' */
    {0x0C, 0x1C, 0x2C, 0x4C, 0x7E, 0x0C, 0x0C, 0x00}, /* 52 '4' */
    {0x7E, 0x60, 0x7C, 0x06, 0x06, 0x66, 0x3C, 0x00}, /* 53 '5' */
    {0x3C, 0x60, 0x60, 0x7C, 0x66, 0x66, 0x3C, 0x00}, /* 54 '6' */
    {0x7E, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00}, /* 55 '7' */
    {0x3C, 0x66, 0x66, 0x3C, 0x66, 0x66, 0x3C, 0x00}, /* 56 '8' */
    {0x3C, 0x66, 0x66, 0x3E, 0x06, 0x0C, 0x38, 0x00}, /* 57 '9' */
    {0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00}, /* 58 ':' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 59 ';' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 60 '<' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 61 '=' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 62 '>' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 63 '?' */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, /* 64 '@' */
    {0x18, 0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x00}, /* 65 'A' */
    {0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00}, /* 66 'B' */
    {0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00}, /* 67 'C' */
    {0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00}, /* 68 'D' */
    {0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x7E, 0x00}, /* 69 'E' */
    {0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x60, 0x00}, /* 70 'F' */
    {0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3C, 0x00}, /* 71 'G' */
    {0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00}, /* 72 'H' */
    {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00}, /* 73 'I' */
    {0x06, 0x06, 0x06, 0x06, 0x66, 0x66, 0x3C, 0x00}, /* 74 'J' */
    {0x66, 0x6C, 0x78, 0x70, 0x78, 0x6C, 0x66, 0x00}, /* 75 'K' */
    {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00}, /* 76 'L' */
    {0x63, 0x77, 0x7F, 0x6B, 0x63, 0x63, 0x63, 0x00}, /* 77 'M' */
    {0x66, 0x76, 0x7E, 0x7E, 0x6E, 0x66, 0x66, 0x00}, /* 78 'N' */
    {0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00}, /* 79 'O' */
    {0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60, 0x60, 0x00}, /* 80 'P' */
    {0x3C, 0x66, 0x66, 0x66, 0x6A, 0x6C, 0x36, 0x00}, /* 81 'Q' */
    {0x7C, 0x66, 0x66, 0x7C, 0x6C, 0x66, 0x66, 0x00}, /* 82 'R' */
    {0x3C, 0x66, 0x60, 0x3C, 0x06, 0x66, 0x3C, 0x00}, /* 83 'S' */
    {0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00}, /* 84 'T' */
    {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00}, /* 85 'U' */
    {0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00}, /* 86 'V' */
    {0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00}, /* 87 'W' */
    {0x66, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x66, 0x00}, /* 88 'X' */
    {0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x00}, /* 89 'Y' */
    {0x7E, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x7E, 0x00}}; /* 90 'Z' */

/* ============================================================================
 * Static Helper Functions
 * ============================================================================
 */

/**
 * \brief Get ANSI color code for class index
 * \param[in] idx Class index
 * \return Pointer to color code string
 */
const char* get_log_color(int idx) {
    switch (idx) {
        case 0: /* class 1 */
            return COLOR_YELLOW;
        case 1: /* class 2 */
            return COLOR_GREEN;
        case 2: /* class 3 */
            return COLOR_RED;
        case 3: /* class 4 */
            return COLOR_BLUE;
        default:
            return COLOR_RESET;
    }
}

/**
 * \brief Draw a single character on an RGB image
 * \param[in,out] image      Pointer to image buffer (RGB888 format)
 * \param[in]     img_width  Image width in pixels
 * \param[in]     img_height Image height in pixels
 * \param[in]     x          X coordinate for character placement
 * \param[in]     y          Y coordinate for character placement
 * \param[in]     c          Character to draw
 * \param[in]     scale      Scaling factor for character size
 */
static void DrawChar(uint8_t* image, uint32_t img_width, uint32_t img_height,
                     uint32_t x, uint32_t y, char c, uint32_t scale) {
    /* Check if character is valid */
    if (c < 0 || c >= ASCII_CHAR_COUNT) {
        return;
    }

    const uint8_t* glyph = font_8x8[(int)c];
    const uint32_t stride = img_width * BYTES_PER_PIXEL_RGB888;

    /* 8x8 font: 8 rows, 8 columns per row */
    for (uint32_t row = 0; row < FONT_HEIGHT; row++) {
        uint8_t bits = glyph[row];
        for (uint32_t col = 0; col < FONT_WIDTH; col++) {
            /* Check if bit is set (MSB = leftmost pixel) */
            if (bits & (FONT_MSB_MASK >> col)) {
                for (uint32_t sy = 0; sy < scale; sy++) {
                    for (uint32_t sx = 0; sx < scale; sx++) {
                        uint32_t py = y + row * scale + sy;
                        uint32_t px_x = x + col * scale + sx;

                        /* Bounds checking */
                        if (py >= img_height || px_x >= img_width) {
                            continue;
                        }

                        uint8_t* px = image + (py * stride) + (px_x * 3);
                        px[0] = 0;   /* Red */
                        px[1] = 0;   /* Green */
                        px[2] = 255; /* Blue */
                    }
                }
            }
        }
    }
}

/**
 * \brief Draw text string on an RGB image
 * \param[in,out] imageBuffer Pointer to image buffer (RGB888 format)
 * \param[in]     img_width   Image width in pixels
 * \param[in]     img_height  Image height in pixels
 * \param[in]     text        Null-terminated text string to draw
 * \param[in]     x           X coordinate for text placement
 * \param[in]     y           Y coordinate for text placement
 * \param[in]     scale       Scaling factor for text size
 */
static void DrawTextOnImage(uint8_t* imageBuffer, uint32_t img_width,
                            uint32_t img_height, const char* text, uint32_t x,
                            uint32_t y, uint32_t scale) {
    const uint32_t char_w = FONT_WIDTH * scale; /* 8x8 font */

    for (uint32_t i = 0; text[i] != '\0'; i++) {
        DrawChar(imageBuffer, img_width, img_height, x + i * char_w, y, text[i],
                 scale);
    }
}

/**
 * \brief Draw classification label centered at bottom of image
 * \param[in,out] imageBuffer Pointer to image buffer (RGB888 format)
 * \param[in]     img_width   Image width in pixels
 * \param[in]     img_height  Image height in pixels
 * \param[in]     text        Null-terminated label text to draw
 */
void DrawClassLabelOnImage(uint8_t* imageBuffer, uint32_t img_width,
                                  uint32_t img_height, const char* text) {
    const uint32_t scale = FONT_SCALE;          /* 2x scaling */
    const uint32_t char_w = FONT_WIDTH * scale; /* 16 pixels wide per char */
    const uint32_t char_height = FONT_HEIGHT * scale; /* 16 pixels tall */

    uint32_t len = strlen(text);
    uint32_t text_width = len * char_w;

    /* Center horizontally in image */
    uint32_t x_start = (img_width - text_width) / 2;

    /* Position from bottom with safe margin */
    /* 224 - 16 - 8 = 200 */
    uint32_t y_start = img_height - char_height - TEXT_BOTTOM_MARGIN;

    DrawTextOnImage(imageBuffer, img_width, img_height, text, x_start, y_start,
                    scale);
}

/**
 * \brief Find index of maximum value in probability array
 * \param[in]  probs      Pointer to probability array
 * \param[in]  n          Number of elements in array
 * \param[out] confidence Pointer to store maximum probability value
 * \return Index of maximum probability
 */
static int argmax(const float* probs, int n, float* confidence) {
    int idx = 0;
    *confidence = probs[0];

    for (int i = 1; i < n; i++) {
        if (probs[i] > *confidence) {
            *confidence = probs[i];
            idx = i;
        }
    }
    return idx;
}

/**
 * \brief Apply softmax function to convert logits to probabilities
 * \param[in]  logits Pointer to logit values array
 * \param[out] probs  Pointer to output probability array
 * \param[in]  n      Number of elements
 */
void softmax(const float* logits, float* probs, int n) {
    float max_val = logits[0];
    for (int i = 1; i < n; i++) {
        if (logits[i] > max_val) {
            max_val = logits[i];
        }
    }

    float sum = 0.0f;
    for (int i = 0; i < n; i++) {
        probs[i] = expf(logits[i] - max_val);
        sum += probs[i];
    }

    for (int i = 0; i < n; i++) {
        probs[i] /= sum;
    }
}

/**
 * \brief Prepare image input tensor for SqueezeNet model
 * \param[in] image Pointer to input image data (HWC RGB format)
 */
void preprocess(const uint8_t* image) {
    /* image layout: HWC, RGBRGB... */
    for (int c = 0; c < C; ++c) {
        for (int h = 0; h < H; ++h) {
            for (int w = 0; w < W; ++w) {
                int hwc_index = (h * W + w) * C + c;
                int nchw_index = c * H * W + h * W + w;

                /* uint8 → float [0,1] */
                float x = image[hwc_index] / 255.0f;

                /* Normalize (ImageNet) */
                x = (x - mean[c]) / stdv[c];

                input_tensor_data[nchw_index] = x;
            }
        }
    }
}

/* ============================================================================
 * ExecuTorch Platform Functions
 * ============================================================================
 */

/**
 * \brief Emit a log message via platform output
 * \param[in] timestamp Timestamp of log message (unused)
 * \param[in] level     Log level
 * \param[in] filename  Source filename
 * \param[in] function  Function name (unused)
 * \param[in] line      Line number
 * \param[in] message   Log message content
 * \param[in] length    Message length (unused)
 */
void et_pal_emit_log_message(ET_UNUSED et_timestamp_t timestamp,
                             et_pal_log_level_t level, const char* filename,
                             ET_UNUSED const char* function, size_t line,
                             const char* message, ET_UNUSED size_t length) {
    fprintf(stdout, "%c [executorch:%s:%u %s()] %s\n", level, filename, line,
            function, message);
}

/**
 * \brief Allocate dynamic memory (not used in bare-metal)
 * \param[in] size Size in bytes to allocate
 * \return Always returns nullptr
 */
void* et_pal_allocate(ET_UNUSED size_t size) { return nullptr; }

/**
 * \brief Free dynamic memory (not used in bare-metal)
 * \param[in] ptr Pointer to free
 */
void et_pal_free(ET_UNUSED void* ptr) {}

/* ============================================================================
 * Anonymous Namespace
 * ============================================================================
 */
namespace {

/**
 * \brief Lightweight heapless container for in-place object construction
 * \tparam T Type to store
 */
template <typename T>
class Box {
   public:
    Box() = default;

    ~Box() {
        if (has_value) {
            ptr()->~T();
        }
    }

    Box(const Box&) = delete;
    Box& operator=(const Box&) = delete;

    /**
     * \brief Destruct existing object and construct new one in-place
     * \tparam Args Constructor argument types
     * \param[in] args Constructor arguments
     */
    template <typename... Args>
    void reset(Args&&... args) {
        if (has_value) {
            reinterpret_cast<T*>(mem)->~T();
        }
        new (mem) T(std::forward<Args>(args)...);
        has_value = true;
    }

    /**
     * \brief Get reference to contained object
     * \return Reference to contained object
     */
    T& value() { return *ptr(); }

    /**
     * \brief Get const reference to contained object
     * \return Const reference to contained object
     */
    const T& value() const { return *ptr(); }

    /**
     * \brief Member access operator
     * \return Pointer to contained object
     */
    T* operator->() { return ptr(); }

    /**
     * \brief Const member access operator
     * \return Const pointer to contained object
     */
    const T* operator->() const { return ptr(); }

   private:
    alignas(T) uint8_t mem[sizeof(T)];
    bool has_value = false;

    /**
     * \brief Get pointer to contained object
     * \return Pointer to contained object
     */
    T* ptr() { return reinterpret_cast<T*>(mem); }

    /**
     * \brief Get const pointer to contained object
     * \return Const pointer to contained object
     */
    const T* ptr() const { return reinterpret_cast<const T*>(mem); }
};

/**
 * \brief Update input tensors with new data
 * \param[in,out] method        Method instance
 * \param[in]     allocator     Memory allocator
 * \param[in]     input_buffers Vector of input buffer pointers and sizes
 * \return Error::Ok on success, error code otherwise
 */
Error update_input_tensors(
    Method& method, MemoryAllocator& allocator,
    const std::vector<std::pair<char*, size_t>>& input_buffers) {
    MethodMeta method_meta = method.method_meta();
    size_t num_inputs = method_meta.num_inputs();

    /* Use stack allocation instead: */
    std::vector<EValue> input_evalues(num_inputs);

    Error err = method.get_inputs(input_evalues.data(), num_inputs);
    ET_CHECK_OK_OR_RETURN_ERROR(err);

    for (size_t i = 0; i < num_inputs; i++) {
        auto tag = method_meta.input_tag(i);
        ET_CHECK_OK_OR_RETURN_ERROR(tag.error());

        if (tag.get() != Tag::Tensor) {
            printf("Skipping non-tensor input %u\n", i);
            continue;
        }

        if (i < input_buffers.size()) {
            auto [buffer, buffer_size] = input_buffers.at(i);
            Result<TensorInfo> tensor_meta = method_meta.input_tensor_meta(i);
            ET_CHECK_OK_OR_RETURN_ERROR(tensor_meta.error());

            if (buffer_size != tensor_meta->nbytes()) {
                printf("Input size (%zu) and tensor size (%zu) mismatch!\n",
                       buffer_size, tensor_meta->nbytes());
                return Error::InvalidArgument;
            }

            if (input_evalues[i].isTensor()) {
                Tensor& tensor = input_evalues[i].toTensor();
                std::memcpy(tensor.mutable_data_ptr<int8_t>(), buffer,
                            buffer_size);
            }
        }
    }

    return Error::Ok;
}

} /* namespace - internal helpers end here */

/**
 * \brief Runner context holding all state for model execution
 */
struct RunnerContext {
    RunnerContext() = default;
    RunnerContext(const RunnerContext& ctx) = delete;
    RunnerContext& operator=(const RunnerContext& ctx) = delete;

    const char* method_name = nullptr;
    size_t planned_buffer_memsize = 0;
    size_t method_loaded_memsize = 0;
    size_t executor_membase = 0;
    size_t program_data_len = 0;
    size_t input_memsize = 0;
    size_t pte_size = 0;
    bool bundle_io = false;
    Box<ArmMemoryAllocator> method_allocator;
    Box<ArmMemoryAllocator> temp_allocator;
    Box<Result<Method>> method;
};

RunnerContext* runner_context_instance(void) {
    static RunnerContext instance;
    return &instance;
}

/**
 * \brief Initialize runner context and load model
 * \param[in,out] ctx           Runner context
 * \param[in]     input_buffers Input buffer specifications
 * \param[in]     pte_size      Size of PTE file
 * \param[in]     program       Loaded program instance
 */
void runner_init(RunnerContext& ctx,
                 std::vector<std::pair<char*, size_t>> input_buffers,
                 size_t pte_size, Program* program) {
    printf("Model buffer loaded, has %u methods\n", program->num_methods());
    {
        const auto method_name_result = program->get_method_name(0);
        ET_CHECK_MSG(method_name_result.ok(), "Program has no methods");
        ctx.method_name = *method_name_result;
    }
    printf("Running method %s\n", ctx.method_name);

    Result<MethodMeta> method_meta = program->method_meta(ctx.method_name);
    if (!method_meta.ok()) {
        printf("Failed to get method_meta for %s: 0x%x\n", ctx.method_name,
               (unsigned int)method_meta.error());
    }

    printf("Setup Method allocator pool. Size: %u bytes.\n",
           method_allocation_pool_size);

    ctx.method_allocator.reset(method_allocation_pool_size,
                               method_allocation_pool);

    std::vector<uint8_t*> planned_buffers;
    std::vector<Span<uint8_t>> planned_spans;
    size_t num_memory_planned_buffers =
        method_meta->num_memory_planned_buffers();

    size_t planned_buffer_membase = ctx.method_allocator->used_size();

    for (size_t id = 0; id < num_memory_planned_buffers; ++id) {
        size_t buffer_size = static_cast<size_t>(
            method_meta->memory_planned_buffer_size(id).get());
        printf("Setting up planned buffer %u, size %u\n", id, buffer_size);

        /* Move to it's own allocator when MemoryPlanner is in place. */
        /* Ethos-U driver requires 16 bit alignment. */
        uint8_t* buffer = reinterpret_cast<uint8_t*>(
            ctx.method_allocator->allocate(buffer_size, 16UL));
        ET_CHECK_MSG(
            buffer != nullptr,
            "Could not allocate memory for memory planned buffer size %u",
            buffer_size);
        planned_buffers.push_back(buffer);
        planned_spans.push_back({planned_buffers.back(), buffer_size});
    }

    ctx.planned_buffer_memsize =
        ctx.method_allocator->used_size() - planned_buffer_membase;

    HierarchicalAllocator planned_memory(
        {planned_spans.data(), planned_spans.size()});

    ctx.temp_allocator.reset(temp_allocation_pool_size, temp_allocation_pool);

    MemoryManager memory_manager(&ctx.method_allocator.value(), &planned_memory,
                                 &ctx.temp_allocator.value());

    size_t method_loaded_membase = ctx.method_allocator->used_size();

    executorch::runtime::EventTracer* event_tracer_ptr = nullptr;

    ctx.method.reset(program->load_method(ctx.method_name, &memory_manager,
                                          event_tracer_ptr));

    if (!ctx.method->ok()) {
        printf("Loading of method %s failed with status 0x%" PRIx32 "\n",
               ctx.method_name, (uint32_t)ctx.method->error());
    }
    ctx.method_loaded_memsize =
        ctx.method_allocator->used_size() - method_loaded_membase;
    printf("Method '%s' loaded.\n", ctx.method_name);

    printf("Model initialized. Ready for inference.\n");

    ctx.executor_membase = ctx.method_allocator->used_size();
}

/**
 * \brief Log memory status and usage statistics
 * \param[in] ctx Runner context
 */
void log_mem_status(RunnerContext& ctx) {
    size_t executor_memsize =
        ctx.method_allocator->used_size() - ctx.executor_membase;

    printf("model_pte_program_size:     %u bytes.\n", ctx.program_data_len);
    printf("model_pte_loaded_size:      %u bytes.\n", ctx.pte_size);
    if (ctx.method_allocator->size() != 0) {
        size_t method_allocator_used = ctx.method_allocator->used_size();
        printf("method_allocator_used:     %u / %u  free: %u ( used: %u %% )\n",
               method_allocator_used, ctx.method_allocator->size(),
               ctx.method_allocator->free_size(),
               100 * method_allocator_used / ctx.method_allocator->size());
        printf("method_allocator_planned:  %u byte\n",
               ctx.planned_buffer_memsize);
        printf("method_allocator_loaded:   %u bytes\n",
               ctx.method_loaded_memsize);
        printf("method_allocator_input:    %u bytes\n", ctx.input_memsize);
        printf("method_allocator_executor: %u bytes\n", executor_memsize);
    }
    if (ctx.temp_allocator->size() > 0) {
        printf("temp_allocator:            %u\n", ctx.temp_allocator->size());
    }
}

/**
 * \brief Print and process model output tensors
 * \param[in] ctx Runner context
 */
void print_outputs(RunnerContext& ctx) {
    std::vector<EValue> outputs(ctx.method.value()->outputs_size());

    Error status =
        ctx.method.value()->get_outputs(outputs.data(), outputs.size());
    ET_CHECK(status == Error::Ok);

    for (int i = 0; i < outputs.size(); ++i) {
        if (outputs[i].isTensor()) {
            Tensor tensor = outputs[i].toTensor();
            if (tensor.scalar_type() != ScalarType::Float) {
                continue;
            }

            const float* logits = tensor.const_data_ptr<float>();
            int numel = tensor.numel();

            float probs[NUM_CLASSES];
            float confidence;
            int predicted_idx;

            softmax(logits, probs, numel);
            predicted_idx = argmax(probs, numel, &confidence);

            printf("\nPost-processed output:\n");
            if (model_config == VEHICLE_MODEL) {
                const char* color = get_log_color(predicted_idx);
                printf("Predicted class : %s%s%s\n", color,
                       VEHICLE_CLASS_NAMES[predicted_idx], COLOR_RESET);
                printf("Confidence      : %.2f %%\n", confidence * PERCENT_SCALE);
                conf_int = (int)(confidence * PERCENT_SCALE);
                conf_score = confidence * PERCENT_SCALE;
                strncpy(label_name, VEHICLE_CLASS_NAMES[predicted_idx],
                        sizeof(label_name) - 1);
                label_name[sizeof(label_name) - 1] = '\0';
                strncpy(output_label.label_name,
                        VEHICLE_CLASS_NAMES[predicted_idx],
                        sizeof(output_label.label_name) - 1);
                output_label.label_name[sizeof(output_label.label_name) - 1] =
                    '\0';
                output_label.confidence = conf_score;

                if (strcmp(label_name, "UNKNOWN") == 0) {
                    classify_object = false;
                } else {
                    classify_object = true;
                }

            } else if (model_config == BANANA_RIPENESS_MODEL) {
                const char* color = get_log_color(predicted_idx);
                printf("Predicted class : %s%s%s\n", color,
                       BANANA_CLASS_NAMES[predicted_idx], COLOR_RESET);
                printf("Confidence      : %.2f %%\n", confidence * PERCENT_SCALE);
                conf_score = confidence * PERCENT_SCALE;
                strncpy(label_name, BANANA_CLASS_NAMES[predicted_idx],
                        sizeof(label_name) - 1);
                label_name[sizeof(label_name) - 1] = '\0';
                strncpy(output_label.label_name,
                        VEHICLE_CLASS_NAMES[predicted_idx],
                        sizeof(output_label.label_name) - 1);
                output_label.label_name[sizeof(output_label.label_name) - 1] =
                    '\0';
                output_label.confidence = conf_score;
            } else if (model_config == TOOL_MODEL) {
                const char* color = get_log_color(predicted_idx);
                printf("Predicted class : %s%s%s\n", color,
                       TOOL_CLASS_NAMES[predicted_idx], COLOR_RESET);
                printf("Confidence      : %.2f %%\n", confidence * PERCENT_SCALE);
                conf_score = confidence * PERCENT_SCALE;
                strncpy(label_name, TOOL_CLASS_NAMES[predicted_idx],
                        sizeof(label_name) - 1);
                label_name[sizeof(label_name) - 1] = '\0';
                strncpy(output_label.label_name,
                        VEHICLE_CLASS_NAMES[predicted_idx],
                        sizeof(output_label.label_name) - 1);
                output_label.label_name[sizeof(output_label.label_name) - 1] =
                    '\0';
                output_label.confidence = conf_score;
            } else if (model_config == RPS_MODEL) {
                const char* color = get_log_color(predicted_idx);
                printf("Predicted class : %s%s%s\n", color,
                       RPS_CLASS_NAMES[predicted_idx], COLOR_RESET);
                printf("Confidence      : %.2f %%\n", confidence * PERCENT_SCALE);
                conf_score = confidence * PERCENT_SCALE;
                conf_int = (int)(confidence * PERCENT_SCALE);
                strncpy(label_name, RPS_CLASS_NAMES[predicted_idx],
                        sizeof(label_name) - 1);
                label_name[sizeof(label_name) - 1] = '\0';
                strncpy(output_label.label_name,
                        RPS_CLASS_NAMES[predicted_idx],
                        sizeof(output_label.label_name) - 1);
                output_label.label_name[sizeof(output_label.label_name) - 1] =
                    '\0';
                output_label.confidence = conf_score;
            } else {
                printf("Invalid classes\n");
            }
        } else {
            printf("Output[%d]: Not Tensor\n", i);
        }
    }
}

/**
 * \brief Full post-processing step: decode outputs, copy result, draw label.
 *
 * Combines print_outputs(), result copy into out_buf, and DrawClassLabelOnImage()
 * into a single call for use in ExecuteAlgorithm().
 *
 * \param[in]     ctx      RunnerContext after a successful run_inference().
 * \param[in]     img_buf  RGB888 frame buffer to draw the label onto.
 * \param[out]    out_buf  Caller buffer to receive runner_output_label_t result.
 * \param[in]     out_num  Byte size of out_buf.
 */
void postprocess(RunnerContext& ctx, uint8_t* img_buf,
                 uint8_t* out_buf, uint32_t out_num) {
    /* Decode output tensor → output_label, conf_int, classify_object */
    print_outputs(ctx);

    /* Copy classification result into caller's output buffer */
    if (out_num >= sizeof(output_label_t)) {
        memcpy(out_buf, &output_label, sizeof(output_label_t));
    }

    /* Format label string and draw onto frame */
    snprintf(output_string, OUTPUT_STRING_SIZE, "%s-%d",
             output_label.label_name, conf_int);
    output_string[OUTPUT_STRING_SIZE - 1] = '\0';

    DrawClassLabelOnImage(img_buf, IMAGE_WIDTH, IMAGE_HEIGHT, output_string);
}

void write_etdump(RunnerContext& ctx) {}

/**
 * \brief Verify model execution results
 * \param[in] ctx       Runner context
 * \param[in] model_pte Pointer to model PTE data
 * \return true if verification passed, false otherwise
 */
bool verify_result(RunnerContext& ctx, const void* model_pte) {
    bool model_ok = false;
#if defined(ET_BUNDLE_IO)
#else  /* defined(ET_BUNDLE_IO) */
    (void)ctx;
    (void)model_pte;
    /* No checking done, assume true */
    model_ok = true;
#endif /* defined(ET_BUNDLE_IO) */
    return model_ok;
}

/**
 * \brief Execute model inference on the pre-processed input tensor.
 *
 * Expects input_tensor_data[] to have already been filled by a prior call
 * to preprocess().  After this function returns
 * successfully, call print_outputs() to decode the results.
 *
 * \param[in,out] ctx  Runner context
 * \return true if execution succeeded, false otherwise
 */
bool run_inference(RunnerContext& ctx) {
    Error status = Error::Ok;
    int n = 0;
    for (n = 0; n < num_inferences; n++) {

        std::vector<std::pair<char*, size_t>> input_buffers;
        input_buffers.emplace_back(reinterpret_cast<char*>(input_tensor_data),
                                   sizeof(input_tensor_data));

        /* Update input tensors with pre-processed frame data */
        status = update_input_tensors(
            *ctx.method.value(), ctx.method_allocator.value(), input_buffers);

        if (status != Error::Ok) {
            printf("Failed to update input tensors: 0x%x\n", status);
            break;
        }

#if ENABLE_TIME_PROFILING
        inference_time = profiler_start();
#endif

        status = ctx.method.value()->execute();
        if (status != Error::Ok) {
            break;
        }
        ctx.temp_allocator.reset(temp_allocation_pool_size,
                                 temp_allocation_pool);

#if ENABLE_TIME_PROFILING
        inference_time = profiler_stop(inference_time);
        printf("Inference time: %3.3f ms.\n",
               profiler_cycles_to_ms(inference_time, CPU_FREQ_HZ));
#endif
    }

    ET_CHECK_MSG(status == Error::Ok,
                 "Execution of method %s failed with status 0x%" PRIx32,
                 ctx.method_name, status);

    return (status == Error::Ok);
}