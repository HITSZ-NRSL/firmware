#if 0
// pull in the sources from the HAL. It's a bit of a hack, but is simpler than trying to link the
// full hal library.
#include "../src/neutron/eeprom_hal.cpp"

#else

#include "eeprom_hal.h"
#include "eeprom_emulation_impl.h"

#endif
