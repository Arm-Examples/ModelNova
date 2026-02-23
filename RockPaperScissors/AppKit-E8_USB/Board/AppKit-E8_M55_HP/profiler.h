#ifndef PROFILER_H
#define PROFILER_H

#include <stdint.h>
#include "RTE_Components.h"
#include CMSIS_device_header

#ifdef __cplusplus
extern "C" {
#endif

#define CPU_FREQ_HZ   (400000000UL)

/** \brief Enable time profiling logs */
#define ENABLE_TIME_PROFILING (0)

/**
 * \brief Initialize cycle counter for profiling
 */
void profiler_init(void);

/**
 * \brief Start profiling section
 * \return cycle counter value at start
 */
static inline uint32_t profiler_start(void)
{
    return DWT->CYCCNT;
}

/**
 * \brief Stop profiling section
 * \param start_cycle cycle count returned by profiler_start()
 * \return elapsed cycles
 */
static inline uint32_t profiler_stop(uint32_t start_cycle)
{
    return (DWT->CYCCNT - start_cycle);
}

/**
 * \brief Convert cycles to milliseconds
 * \param cycles elapsed cycles
 * \param cpu_freq_hz CPU frequency in Hz
 * \return time in milliseconds
 */
double profiler_cycles_to_ms(uint32_t cycles, uint32_t cpu_freq_hz);

#ifdef __cplusplus
}
#endif

#endif /* PROFILER_H */
