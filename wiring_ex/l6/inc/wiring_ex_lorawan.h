#ifndef  WIRING_EX_LORAWAN_H_
#define  WIRING_EX_LORAWAN_H_


#include "../src/lorawan/radio/inc/radio.h"
#include "../src/lorawan/board/inc/sx1278-board.h"

#include "../src/lorawan/radio/inc/sx1278.h"
#include "../src/lorawan/board/inc/timer.h"
#include "../src/lorawan/mac/inc/LoRaMac.h"

#include "../src/lorawan/board/inc/utilities.h"

bool SX1278Test(int8_t &snr, int8_t &rssi, int8_t &txRssi);

#endif /* WIRING_LORAWAN_H_ */

