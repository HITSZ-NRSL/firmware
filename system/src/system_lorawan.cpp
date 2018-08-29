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

#include "intorobot_config.h"

#include <stdlib.h>
#include <stdio.h>
#include "hw_config.h"
#include "core_hal.h"
#include "params_hal.h"
#include "system_datapoint.h"
#include "system_mode.h"
#include "system_task.h"
#include "system_rgbled.h"
#include "system_lorawan.h"
#include "system_product.h"
#include "system_utilities.h"
#include "string_convert.h"
#include "wiring_system.h"
#include "wiring_interrupts.h"

const static char *TAG = "sys-lorawan";

using namespace intorobot;

#ifndef configNO_LORAWAN

#define LORAWAN_PUBLIC_NETWORK                  true
#define LORAWAN_NETWORK_ID                      ( uint32_t )0

volatile bool INTOROBOT_LORAWAN_JOINED          = false;   //lorawan激活通过
volatile bool INTOROBOT_LORAWAN_CONNECTED       = false;   //lorawan发送版本信息完毕 已连接平台
volatile bool INTOROBOT_LORAWAN_JOIN_ENABLE     = false;   //入网使能 true使能
volatile bool INTOROBOT_LORAWAN_JOINING         = false;   //入网中
volatile bool INTOROBOT_LORAWAN_RESP_SERVER_ACK = false;   //c类时回复服务器确认帧

static LoRaMacPrimitives_t LoRaMacPrimitives;
static LoRaMacCallback_t LoRaMacCallbacks;
static RadioEvents_t loraRadioEvents;

static void OnLoRaRadioTxDone(void)
{
    LoRa._radioSendStatus = 0;
    LoRa._radioRunStatus = ep_lora_radio_tx_done;
    system_notify_event(event_lora_radio_status,ep_lora_radio_tx_done);
}

static void OnLoRaRadioTxTimeout(void)
{
    LoRa._radioSendStatus = -1;
    LoRa._radioRunStatus = ep_lora_radio_tx_fail;
    system_notify_event(event_lora_radio_status,ep_lora_radio_tx_fail);
}

static void OnLoRaRadioRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
    LoRa._rssi = rssi;
    LoRa._snr = snr;
    LoRa._bufferSize = size;
    LoRa._available = true;
    if(LoRa._buffer != NULL) {
        free(LoRa._buffer);
        LoRa._buffer = NULL;
    }
    LoRa._buffer = (uint8_t *)malloc(LoRa._bufferSize);
    memcpy( LoRa._buffer, payload, LoRa._bufferSize);
    LoRa._radioRunStatus = ep_lora_radio_rx_done;
    system_notify_event(event_lora_radio_status,ep_lora_radio_rx_done,payload,size);
}

static void OnLoRaRadioRxTimeout(void)
{
    LoRa._radioRunStatus = ep_lora_radio_rx_timeout;
    system_notify_event(event_lora_radio_status,ep_lora_radio_rx_timeout);
}

static void OnLoRaRadioRxError(void)
{
    LoRa._radioRunStatus = ep_lora_radio_rx_error;
    system_notify_event(event_lora_radio_status,ep_lora_radio_rx_error);
}

static void OnLoRaRadioCadDone(bool channelActivityDetected)
{
    if(channelActivityDetected) {
        LoRa._radioRunStatus = ep_lora_radio_cad_detected;
    } else {
        LoRa._radioRunStatus = ep_lora_radio_cad_done;
    }
}

static void McpsConfirm( McpsConfirm_t *mcpsConfirm )
{
    if( mcpsConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK ) {
        LoRaWan._macSendStatus = LORAMAC_SEND_OK;
        switch( mcpsConfirm->McpsRequest ) {
            case MCPS_UNCONFIRMED:
                {
                    // Check Datarate
                    // Check TxPower
                    // DEBUG("mcpsConfirm->MCPS_UNCONFIRMED");
                    LoRaWan._uplinkDatarate = mcpsConfirm->Datarate;
                    LoRaWan._txPower = mcpsConfirm->TxPower;
                    LoRaWan._macRunStatus = ep_lorawan_mcpsconfirm_unconfirmed;
                    if(!INTOROBOT_LORAWAN_RESP_SERVER_ACK) {
                        system_notify_event(event_lorawan_status,ep_lorawan_send_success);
                    }
                    break;
                }
            case MCPS_CONFIRMED:
                {
                    // Check Datarate
                    // Check TxPower
                    // Check AckReceived
                    // Check NbTrials
                    // DEBUG("mcpsConfirm->MCPS_CONFIRMED");
                    LoRaWan._uplinkDatarate = mcpsConfirm->Datarate;
                    LoRaWan._txPower = mcpsConfirm->TxPower;
                    LoRaWan._ackReceived = mcpsConfirm->AckReceived;
                    LoRaWan._macRunStatus = ep_lorawan_mcpsconfirm_confirmed;
                    if(!LoRaWan._ackReceived) {
                        LoRaWan._macSendStatus = LORAMAC_SEND_FAIL;
                        system_notify_event(event_lorawan_status, ep_lorawan_send_fail);
                    } else {
                        system_notify_event(event_lorawan_status, ep_lorawan_send_success);
                    }
                    break;
                }
            case MCPS_PROPRIETARY:
                {
                    break;
                }
            default:
                break;
        }
    } else {
        if(mcpsConfirm->McpsRequest == MCPS_CONFIRMED) {
            LoRaWan._macRunStatus = ep_lorawan_mcpsconfirm_confirmed;
            LoRaWan._ackReceived = false;
        }
        LoRaWan._macSendStatus = LORAMAC_SEND_FAIL;
        if(!INTOROBOT_LORAWAN_RESP_SERVER_ACK) {
            system_notify_event(event_lorawan_status,ep_lorawan_send_fail);
        }
    }
}

static void McpsIndication( McpsIndication_t *mcpsIndication )
{
    if( mcpsIndication->Status != LORAMAC_EVENT_INFO_STATUS_OK ) {
        return;
    }

    // Check Multicast
    // Check Port
    // Check Datarate
    // Check FramePending
    // Check Buffer
    // Check BufferSize
    // Check Rssi
    // Check Snr
    // Check RxSlot
    LoRaWan._downlinkDatarate = mcpsIndication->RxDatarate;
    LoRaWan._macRssi = mcpsIndication->Rssi;
    LoRaWan._macSnr = mcpsIndication->Snr;
    LoRaWan._framePending = mcpsIndication->FramePending;

    if( mcpsIndication->RxData == true ) {
        LoRaWan._available = true;
        LoRaWan._bufferSize = mcpsIndication->BufferSize;
        if(LoRaWan._buffer != NULL) {
            free(LoRaWan._buffer);
            LoRaWan._buffer = NULL;
        }
        LoRaWan._buffer = (uint8_t *)malloc(LoRaWan._bufferSize);
        memcpy(LoRaWan._buffer, mcpsIndication->Buffer, mcpsIndication->BufferSize);

        intorobotParseReceiveDatapoints(mcpsIndication->Buffer, mcpsIndication->BufferSize);
        system_notify_event(event_cloud_comm, ep_cloud_comm_data, mcpsIndication->Buffer, mcpsIndication->BufferSize);

        LoRaWanOnEvent(LORAWAN_EVENT_RX_COMPLETE);
    }

    switch( mcpsIndication->McpsIndication ) {
        case MCPS_UNCONFIRMED:
            {
                break;
            }
        case MCPS_CONFIRMED:
            {
                LoRaWanOnEvent(LORAWAN_EVENT_MCPSINDICATION_CONFIRMED);
                break;
            }
        case MCPS_PROPRIETARY:
            {
                break;
            }
        case MCPS_MULTICAST:
            {
                break;
            }
        default:
            break;
    }
}

static void MlmeConfirm( MlmeConfirm_t *mlmeConfirm )
{
    switch( mlmeConfirm->MlmeRequest ) {
        case MLME_JOIN:
            {
                if( mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK ) {
                    // Status is OK, node has joined the network
                    if(!System.featureEnabled(SYSTEM_FEATURE_STANDARD_LORAWAN_ENABLED)) {
                        uint16_t channelMask = 0;
                        uint8_t lastChannel = LoRaMacGetLastTxChannel();
                        channelMask = 1 << lastChannel;

                        MibRequestConfirm_t mibReq;
                        mibReq.Type = MIB_CHANNELS_DEFAULT_MASK;
                        mibReq.Param.ChannelsDefaultMask = &channelMask;
                        LoRaMacMibSetRequestConfirm( &mibReq );

                        mibReq.Type = MIB_CHANNELS_MASK;
                        mibReq.Param.ChannelsMask = &channelMask;
                        LoRaMacMibSetRequestConfirm( &mibReq );
                        MOLMC_LOGD(TAG, "lorawan close other channel");
                    }

                    LoRaWanOnEvent(LORAWAN_EVENT_JOINED);
                } else {
                    // Join was not successful. Try to join again
                    LoRaWanOnEvent(LORAWAN_EVENT_JOIN_FAIL);
                }
                break;
            }
        case MLME_LINK_CHECK:
            {
                if( mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK ) {
                    // Check DemodMargin
                    // Check NbGateways
                    LoRaWan._demodMargin = mlmeConfirm->DemodMargin;
                    LoRaWan._nbGateways = mlmeConfirm->NbGateways;
                }
                break;
            }
        default:
            break;
    }
}

void os_getDevEui(uint8_t *buf)
{
    char deveui[24]={0};

    HAL_PARAMS_Get_System_device_id(deveui, sizeof(deveui));
    string2hex(deveui, buf, 8, true);
}

void os_getAppEui(uint8_t *buf)
{
    char appeui[24] = {0};
    char temp[24] = {0};

    HAL_PARAMS_Get_System_appeui(appeui, sizeof(appeui));
    system_get_product_id(temp, sizeof(temp));
    //lora产品将product_id当做appeui
    if(strcmp(temp, appeui) != 0) {
        strncpy(appeui,temp,strlen(temp));
        HAL_PARAMS_Set_System_appeui(appeui);
    }
    string2hex(appeui, buf, 8, true);
}

void os_getAppKey(uint8_t *buf)
{
    char appkey[36]={0};

    HAL_PARAMS_Get_System_appkey(appkey, sizeof(appkey));
    string2hex(appkey, buf, 16, false);
}

void LoRaWanJoinEnable(bool enable)
{
    INTOROBOT_LORAWAN_JOIN_ENABLE = enable;
}

bool LoRaWanJoinIsEnabled(void)
{
    return INTOROBOT_LORAWAN_JOIN_ENABLE;
}

int LoRaWanActiveStatus(void)
{
    if(INTOROBOT_LORAWAN_JOINED && INTOROBOT_LORAWAN_CONNECTED) {
        return 0;
    } else {
        if(INTOROBOT_LORAWAN_JOINING) {
            return 1;
        }
    }
    return -1;
}

void LoRaWanDisconnect(void)
{
    INTOROBOT_LORAWAN_JOINED = false;
    INTOROBOT_LORAWAN_JOINING = false;
    INTOROBOT_LORAWAN_JOIN_ENABLE = false;
    INTOROBOT_LORAWAN_CONNECTED = false;
    system_rgb_blink(RGB_COLOR_GREEN, 1000);//绿灯闪烁
}

void LoRaWanPause(void)
{
    System.disableFeature(SYSTEM_FEATURE_LORAMAC_RUN_ENABLED);
    LoRaWanJoinEnable(false);
    // Radio initialization
    // SX1276BoardInit();
    noInterrupts();
    SX1276IoIrqDeInit();
    loraRadioEvents.TxDone = OnLoRaRadioTxDone;
    loraRadioEvents.RxDone = OnLoRaRadioRxDone;
    loraRadioEvents.TxTimeout = OnLoRaRadioTxTimeout;
    loraRadioEvents.RxTimeout = OnLoRaRadioRxTimeout;
    loraRadioEvents.RxError = OnLoRaRadioRxError;
    loraRadioEvents.CadDone = OnLoRaRadioCadDone;
    Radio.Init(&loraRadioEvents);
    interrupts();
    Radio.SetModem(MODEM_LORA);
}

void LoRaWanResume(void)
{
    System.enableFeature(SYSTEM_FEATURE_LORAMAC_RUN_ENABLED);
    SX1276BoardInit(); //初始化1278
    LoRaMacPrimitives.MacMcpsConfirm = McpsConfirm;
    LoRaMacPrimitives.MacMcpsIndication = McpsIndication;
    LoRaMacPrimitives.MacMlmeConfirm = MlmeConfirm;
    LoRaMacCallbacks.GetBatteryLevel = BoardGetBatteryLevel;
    LoRaMacInitialization( &LoRaMacPrimitives, &LoRaMacCallbacks, LORAMAC_REGION_EU433);

    MibRequestConfirm_t mibReq;
    //设置ADR
    mibReq.Type = MIB_ADR;
    mibReq.Param.AdrEnable = LoRaWan.getAdrOn();
    LoRaMacMibSetRequestConfirm(&mibReq);
    //设置为公共网络
    mibReq.Type = MIB_PUBLIC_NETWORK;
    mibReq.Param.EnablePublicNetwork = LORAWAN_PUBLIC_NETWORK;
    LoRaMacMibSetRequestConfirm(&mibReq);
}

void LoRaWanJoinOTAA(void)
{
    MlmeReq_t mlmeReq;
    uint8_t _devEui[8];
    uint8_t _appEui[8];

    os_getDevEui(_devEui);
    os_getAppEui(_appEui);
    os_getAppKey(LoRaWan.macParams.appKey);

    memcpyr(LoRaWan.macParams.devEui,_devEui,8);
    memcpyr(LoRaWan.macParams.appEui,_appEui,8);

    mlmeReq.Type = MLME_JOIN;
    mlmeReq.Req.Join.DevEui = LoRaWan.macParams.devEui;
    mlmeReq.Req.Join.AppEui = LoRaWan.macParams.appEui;
    mlmeReq.Req.Join.AppKey = LoRaWan.macParams.appKey;
    mlmeReq.Req.Join.NbTrials = LoRaWan._joinNbTrials;

    if( LoRaMacMlmeRequest( &mlmeReq ) != LORAMAC_STATUS_OK ) {
        LoRaWanOnEvent(LORAWAN_EVENT_JOIN_FAIL);
    }
}

bool LoRaWanJoinABP(void)
{
    MibRequestConfirm_t mibReq;
    char devaddr[16] = {0}, nwkskey[36] = {0}, appskey[36] = {0};

    HAL_PARAMS_Get_System_devaddr(devaddr, sizeof(devaddr));
    HAL_PARAMS_Get_System_nwkskey(nwkskey, sizeof(nwkskey));
    HAL_PARAMS_Get_System_appskey(appskey, sizeof(appskey));
    string2hex(devaddr, (uint8_t *)&LoRaWan.macParams.devAddr, 4, true);
    string2hex(nwkskey, LoRaWan.macParams.nwkSkey, 16, false);
    string2hex(appskey, LoRaWan.macParams.appSkey, 16, false);

    mibReq.Type = MIB_NET_ID;
    mibReq.Param.NetID = LORAWAN_NETWORK_ID;
    LoRaMacMibSetRequestConfirm(&mibReq);

    mibReq.Type = MIB_DEV_ADDR;
    mibReq.Param.DevAddr = LoRaWan.macParams.devAddr;
    LoRaMacMibSetRequestConfirm(&mibReq);

    mibReq.Type = MIB_NWK_SKEY;
    mibReq.Param.NwkSKey = LoRaWan.macParams.nwkSkey;
    LoRaMacMibSetRequestConfirm(&mibReq);

    mibReq.Type = MIB_APP_SKEY;
    mibReq.Param.AppSKey = LoRaWan.macParams.appSkey;
    LoRaMacMibSetRequestConfirm(&mibReq);

    mibReq.Type = MIB_NETWORK_JOINED;
    mibReq.Param.IsNetworkJoined = true;
    LoRaMacMibSetRequestConfirm(&mibReq);

    INTOROBOT_LORAWAN_CONNECTED = false;
    INTOROBOT_LORAWAN_JOINED = true;
    system_rgb_blink(RGB_COLOR_WHITE, 2000); //白灯闪烁
    return true;
}

void LoRaWanGetABPParams(uint32_t &devAddr, uint8_t *nwkSkey, uint8_t *appSkey)
{
    MibRequestConfirm_t mibReq;
    LoRaMacStatus_t status;

    mibReq.Type = MIB_DEV_ADDR;
    status = LoRaMacMibGetRequestConfirm(&mibReq);
    if(status == LORAMAC_STATUS_OK) {
        devAddr = mibReq.Param.DevAddr;
    }

    mibReq.Type = MIB_NWK_SKEY;
    status = LoRaMacMibGetRequestConfirm(&mibReq);
    if(status == LORAMAC_STATUS_OK) {
        memcpy(nwkSkey,mibReq.Param.NwkSKey,16);
    }

    mibReq.Type = MIB_APP_SKEY;
    status = LoRaMacMibGetRequestConfirm(&mibReq);
    if(status == LORAMAC_STATUS_OK) {
        memcpy(appSkey,mibReq.Param.AppSKey,16);
    }
}

//回复平台控制类型的数据点来通知服务器节点已收到
//发送空的确认包
void LoRaWanRespondServerConfirmedFrame(void)
{
    McpsReq_t mcpsReq;
    mcpsReq.Type = MCPS_UNCONFIRMED;
    mcpsReq.Req.Unconfirmed.fBuffer = NULL;
    mcpsReq.Req.Unconfirmed.fBufferSize = 0;
    mcpsReq.Req.Unconfirmed.Datarate = LoRaWan.getDataRate();

    if( LoRaMacMcpsRequest( &mcpsReq ) == LORAMAC_STATUS_OK ) {
        MOLMC_LOGD(TAG, "LoRaWan send empty frame status OK!!!");
    }
}

bool intorobot_lorawan_flag_connected(void)
{
    if (INTOROBOT_LORAWAN_JOINED && INTOROBOT_LORAWAN_CONNECTED) {
        return true;
    } else {
        return false;
    }
}

int intorobot_lorawan_send_data(char* buffer, uint16_t len, bool confirmed, uint16_t timeout)
{
    if(intorobot_lorawan_flag_connected()) {
        if(confirmed) {
            return LoRaWan.sendConfirmed(2, buffer, len, timeout);
        } else {
            return LoRaWan.sendUnconfirmed(2, buffer, len, timeout);
        }
    }
    return -1;
}

void LoRaWanOnEvent(lorawan_event_t event)
{
    //主从模式下都由内部存储参数
    switch(event) {
        case LORAWAN_EVENT_JOINED:
            {
                char devaddr[16] = "", nwkskey[36] = "", appskey[36] = "";
                uint32_t devAddr;
                uint8_t nwkSkey[16],appSkey[16];

                LoRaWanGetABPParams(devAddr,nwkSkey,appSkey);
                //devaddr
                hex2string((uint8_t *)&devAddr, 4, devaddr, true);
                HAL_PARAMS_Set_System_devaddr(devaddr);
                //nwkskey
                hex2string(nwkSkey, 16, nwkskey, false);
                HAL_PARAMS_Set_System_nwkskey(nwkskey);
                //appskey
                hex2string(appSkey, 16, appskey, false);
                HAL_PARAMS_Set_System_appskey(appskey);
                HAL_PARAMS_Set_System_at_mode(AT_MODE_FLAG_OTAA_ACTIVE);
                HAL_PARAMS_Save_Params();

                MOLMC_LOGD(TAG, "devaddr: %s", devaddr);
                MOLMC_LOGD(TAG, "nwkskey: %s", nwkskey);
                MOLMC_LOGD(TAG, "appskey: %s", appskey);
                INTOROBOT_LORAWAN_JOINED = true;
                INTOROBOT_LORAWAN_CONNECTED = true;

                LoRaWan._macRunStatus = ep_lorawan_join_success;
                system_notify_event(event_lorawan_status,ep_lorawan_join_success);

                system_rgb_blink(RGB_COLOR_WHITE, 2000); //白灯闪烁
                MOLMC_LOGD(TAG, "--LoRaWanOnEvent joined--");
            }
            break;

        case LORAWAN_EVENT_JOIN_FAIL:
            INTOROBOT_LORAWAN_JOINING = false;

            LoRaWan._macRunStatus = ep_lorawan_join_fail;
            system_notify_event(event_lorawan_status,ep_lorawan_join_fail);

            MOLMC_LOGD(TAG, "--LoRaWanOnEvent join failed--");
            break;

        case LORAWAN_EVENT_RX_COMPLETE:
            break;

        case LORAWAN_EVENT_MCPSINDICATION_CONFIRMED:
            MOLMC_LOGD(TAG, "LoRaWanOnEvent Respond Server ACK");
            if(LoRaWan.getMacClassType() == CLASS_C) {
                INTOROBOT_LORAWAN_RESP_SERVER_ACK = true;
                LoRaWanRespondServerConfirmedFrame();
            }
            break;

        default:
            break;
    }
}
#endif

