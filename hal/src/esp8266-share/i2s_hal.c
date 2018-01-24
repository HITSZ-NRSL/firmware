/**
 ******************************************************************************
  Copyright (c) 2013-2014 IntoRobot Team.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "i2s_hal.h"
#include "service_debug.h"

/*
 * @brief Initial the I2S, include setup which I2S used, and some flags.
 * @param i2s: i2s number chosed
 * @retral None
 */
void HAL_I2S_Initial(HAL_I2S_Interface i2s)
{

}

/*
 * @brief Begin the I2S
 * @param i2s: i2s number chosed
 * @param pin: NSS pin, not used
 * @retral None
 */
void HAL_I2S_Begin(HAL_I2S_Interface i2s, uint32_t audioFreq)
{

}

/*
 * @brief End the I2S.
 * @param i2s: i2s number chosed
 * @retral None
 */
void HAL_I2S_End(HAL_I2S_Interface i2s)
{

}

/*
 * @brief I2S recieve data.
 * @param i2s: i2s number chosed.
 * @retral The received data.
 */
uint16_t HAL_I2S_DMA_Receive_Data(HAL_I2S_Interface i2s)
{
    return 0;
}

/*
 * @brief Check whether I2S is enabled or not.
 * @param i2s: i2s number chosed.
 * @retral The enabled flag: true, i2s have begin; false, the i2s not working.
 */
bool HAL_I2S_Is_Enabled(HAL_I2S_Interface i2s)
{
    return false;
}
