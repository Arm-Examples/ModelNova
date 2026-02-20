#include "profiler.h"

/**
 * \brief Initialize the CPU cycle counter for profiling.
 *
 * This function enables the Trace and Debug block, resets the
 * Data Watchpoint and Trace (DWT) cycle counter, and starts
 * counting CPU cycles. It must be called once before using
 * cycle-based profiling measurements.
 *
 * \return void
 */
void profiler_init(void)
{
    /* Enable TRC */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    /* Reset cycle counter */
    DWT->CYCCNT = 0;

    /* Enable cycle counter */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/**
 * \brief Convert CPU cycle count to time in milliseconds.
 *
 * This function converts a given number of CPU cycles into
 * milliseconds based on the provided CPU clock frequency.
 *
 * \param cycles        Number of CPU cycles measured.
 * \param cpu_freq_hz   CPU clock frequency in Hertz.
 *
 * \return Time in milliseconds corresponding to the cycle count.
 */
double profiler_cycles_to_ms(uint32_t cycles, uint32_t cpu_freq_hz)
{
    return ((double)cycles * 1000.0) / (double)cpu_freq_hz;
}
