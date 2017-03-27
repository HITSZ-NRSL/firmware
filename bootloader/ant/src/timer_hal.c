#if 0
// pull in the sources from the HAL. It's a bit of a hack, but is simpler than trying to link the
// full hal library.
#include "../src/lora/timer_hal.c"

#else

#include "hw_config.h"
#include "timer_hal.h"


/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/*
 * @brief Should return the number of microseconds since the processor started up.
 */
system_tick_t HAL_Timer_Get_Micro_Seconds(void)
{
    return GetSystem1UsTick();
}

/*
 * @brief Should return the number of milliseconds since the processor started up.
 */
system_tick_t HAL_Timer_Get_Milli_Seconds(void)
{
    return GetSystem1MsTick();
}

#endif
