/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef SYSTEM_TASK_H_
#define SYSTEM_TASK_H_

#include "socket_hal.h"
#include "system_cloud.h"
#include "wlan_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

void system_process_loop(void);
void ui_process_loop(void);

uint32_t HAL_NET_SetNetWatchDog(uint32_t timeOutInuS);
void Network_Setup();
void LoraWAN_Setup(void);

extern volatile uint8_t g_intorobot_network_connected;
extern volatile uint8_t g_intorobot_cloud_pregrocessed;
extern volatile uint8_t g_intorobot_cloud_connected;

extern volatile uint8_t g_intorobot_lorawan_joined;
extern volatile uint8_t g_intorobot_lorawan_connected;

extern volatile uint8_t INTOROBOT_WLAN_SLEEP;
extern volatile uint8_t INTOROBOT_WLAN_STARTED;

extern volatile uint8_t SYSTEM_POWEROFF;
extern volatile uint32_t TimingFlashUpdateTimeout;

extern volatile uint8_t intorobot_process_flag;

/**
 * Determines the backoff period after a number of failed connections.
 */
unsigned backoff_period(unsigned connection_attempts);

/**
 * This is for internal testing. Do not call this function since it is not
 * guaranteed to be preserved or backwards compatible between releases.
 */
void* system_internal(int item, void* reserved);

void system_delay_ms(unsigned long ms, bool force_no_background_loop=false);

#define INTOROBOT_LOOP_DELAY_MILLIS                 1000    //1sec

#ifdef __cplusplus
}
#endif

#endif  /*SYSTEM_TASK_H_*/
