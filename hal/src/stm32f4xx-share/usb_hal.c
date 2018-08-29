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
#include "usb_hal.h"
#include "tick_hal.h"
#include "hw_config.h"
#include "sdkqueue.h"

/* Private typedef -----------------------------------------------------------*/
#ifdef configHAL_USB_CDC_ENABLE

/* Private define ------------------------------------------------------------*/

//TODO
//static osMutexId usb_mutex;	//transfer all mutex

/*******************************************************************************
 * Function Name  : USB_USART_Init
 * Description    : Start USB-USART protocol.
 * Input          : baudRate.
 * Return         : None.
 *******************************************************************************/
void USB_USART_Initial(uint32_t baudRate)
{
    //TODO
    //create usb mutex
    //osMutexDef(USB_MUT);
    //usb_mutex = osMutexCreate(osMutex(USB_MUT));
    if (LineCoding.bitrate != baudRate) {
        if (!baudRate && LineCoding.bitrate) {
            USBD_Stop(&USBD_Device);
            USBD_DeInit(&USBD_Device);
        } else if (!LineCoding.bitrate) {
            /* Init Device Library */
            USBD_Init(&USBD_Device, &VCP_Desc, 0);
            /* Add Supported Class */
            USBD_RegisterClass(&USBD_Device, USBD_CDC_CLASS);
            /* Add CDC Interface Class */
            USBD_CDC_RegisterInterface(&USBD_Device, &USBD_CDC_fops);
            /* Start Device Process */
            USBD_Start(&USBD_Device);
        }
        //LineCoding.bitrate will be overwritten by USB Host
        LineCoding.bitrate = baudRate;
    }
}

unsigned USB_USART_Baud_Rate(void)
{
    return LineCoding.bitrate;
}

void USB_USART_LineCoding_BitRate_Handler(void (*handler)(uint32_t bitRate))
{
    //Init USB Serial first before calling the linecoding handler
    USB_USART_Initial(115200);

    //Set the system defined custom handler
    SetLineCodingBitRateHandler(handler);
}

static inline bool USB_USART_Connected() {
    return LineCoding.bitrate > 0;
}

/*******************************************************************************
 * Function Name  : USB_USART_Available_Data.
 * Description    : Return the length of available data received from USB.
 * Input          : None.
 * Return         : Length.
 *******************************************************************************/
int32_t USB_USART_Available_Data(void)
{
    return sdkGetQueueDataLen(&USB_Rx_Queue);
}

/*******************************************************************************
 * Function Name  : USB_USART_Receive_Data.
 * Description    : Return data sent by USB Host.
 * Input          : None
 * Return         : Data.
 *******************************************************************************/
int32_t USB_USART_Receive_Data(uint8_t peek)
{
    uint8_t data;

    if(USB_USART_Available_Data()) {
        if(peek) {
            sdkTryQueueData(&USB_Rx_Queue, sdkGetQueueHead(&USB_Rx_Queue) , &data);
        } else {
            sdkGetQueueData(&USB_Rx_Queue, &data);
        }
        return data;
    }
    return -1;
}

/*******************************************************************************
 * Function Name  : USB_USART_Available_Data_For_Write.
 * Description    : Return the length of available space in TX buffer
 * Input          : None.
 * Return         : Length.
 *******************************************************************************/
int32_t USB_USART_Available_Data_For_Write(void)
{
    if (USB_USART_Connected()) {
        return 1;
    }
    return -1;
}

/*******************************************************************************
 * Function Name  : USB_USART_Send_Data.
 * Description    : Send Data from USB_USART to USB Host.
 * Input          : Data.
 * Return         : None.
 *******************************************************************************/
void USB_USART_Send_Data(uint8_t Data)
{
    //TODO
    //osMutexWait(usb_mutex, osWaitForever);
    volatile system_tick_t start_micros, current_micros, elapsed_micros;

    start_micros = HAL_Tick_Get_Micro_Seconds();
    if (USBD_STATE_CONFIGURED == USBD_Device.dev_state) {
        USBD_CDC_SetTxBuffer(&USBD_Device, &Data, 1);
        while(USBD_CDC_TransmitPacket(&USBD_Device) != USBD_OK) {
            current_micros = HAL_Tick_Get_Micro_Seconds();
            elapsed_micros = current_micros - start_micros;

            if (elapsed_micros < 0){
                elapsed_micros =  0xFFFFFFFF - start_micros + current_micros;
            }

            if (elapsed_micros >= 2000) {  //2000us 超时
                break;
            }
        }
    }

    //TODO
    //osMutexRelease(usb_mutex);
}

void USB_USART_Flush_Data(void)
{
}

/**
 * @brief  This function handles USB-On-The-Go HS/FS global interrupt request.
 * @param  None
 * @retval None
 */
void OTG_FS_IRQHandler(void)
{
    HAL_PCD_IRQHandler(&hpcd);
}

#endif

#ifdef configHAL_USB_HID_ENABLE
/*******************************************************************************
 * Function Name : USB_HID_Send_Report.
 * Description   : Send HID Report Info to Host.
 * Input         : pHIDReport and reportSize.
 * Output        : None.
 * Return value  : None.
 *******************************************************************************/
void USB_HID_Send_Report(void *pHIDReport, size_t reportSize)
{
}
#endif
