#include "intorobot_config.h"
#ifndef configNO_LORAWAN

#include "wiring_lorawan.h"
#include "system_lorawan.h"
#include "system_event.h"
#include "wiring_system.h"
#include "wiring_cloud.h"
#include "string_convert.h"
#include <stdlib.h>
#include "service_debug.h"

#define WIRING_LORAWAN_DEBUG

#ifdef WIRING_LORAWAN_DEBUG
#define WLORAWAN_DEBUG(...)    do {DEBUG(__VA_ARGS__);}while(0)
#define WLORAWAN_DEBUG_D(...)  do {DEBUG_D(__VA_ARGS__);}while(0)
#define WLORAWAN_DEBUG_DUMP    DEBUG_DUMP
#else
#define WLORAWAN_DEBUG(...)
#define WLORAWAN_DEBUG_D(...)
#define WLORAWAN_DEBUG_DUMP
#endif

LoRaWanClass LoRaWan;
LoRaClass LoRa;
//lorawan 接口
bool LoRaWanClass::joinABP(void)
{
    return LoRaWanJoinABP();
}

int LoRaWanClass::joinOTAA(uint16_t timeout)
{
    uint32_t _timeout = timeout;
    LoRaWanJoinEnable(true);

    if(_timeout == 0){
        return 1;
    }
    if(_timeout < 180){
        _timeout = 180;
    }

    _macRunStatus = 0;
    uint32_t prevTime = millis();
    while(1){
        if(_macRunStatus == ep_lorawan_join_success){
            return 0;
        }else if(_macRunStatus == ep_lorawan_join_fail){
            return -1;
        }

        if(millis() - prevTime >= _timeout * 1000){
            IntoYun.disconnect();
            return -1;
        }
        intorobot_process();
    }
}

int LoRaWanClass::sendConfirmed(uint8_t port, uint8_t *buffer, uint16_t len, uint16_t timeout)
{
    McpsReq_t mcpsReq;
    LoRaMacTxInfo_t txInfo;
    uint32_t _timeout = timeout;

    LoRaMacStatus_t loramacStatus = LoRaMacQueryTxPossible( len, &txInfo ) ;

    if(loramacStatus != LORAMAC_STATUS_OK) {
        WLORAWAN_DEBUG("LoRaWan send empty frame\r\n");
        // Send empty frame in order to flush MAC commands
        mcpsReq.Type = MCPS_UNCONFIRMED;
        mcpsReq.Req.Unconfirmed.fBuffer = NULL;
        mcpsReq.Req.Unconfirmed.fBufferSize = 0;
        mcpsReq.Req.Unconfirmed.Datarate = _macDatarate;
    } else {
        // WLORAWAN_DEBUG("LoRaWan send confirmed frame\r\n");
        mcpsReq.Type = MCPS_CONFIRMED;
        mcpsReq.Req.Confirmed.fPort = port;
        mcpsReq.Req.Confirmed.fBuffer = buffer;
        mcpsReq.Req.Confirmed.fBufferSize = len;
        mcpsReq.Req.Confirmed.NbTrials = _confirmedNbTrials;
        mcpsReq.Req.Confirmed.Datarate = _macDatarate;
    }

    if( LoRaMacMcpsRequest( &mcpsReq ) == LORAMAC_STATUS_OK ) {
        WLORAWAN_DEBUG("LoRaWan send confirm frame status OK!!!\r\n");
        _macSendStatus = LORAMAC_SENDING;
        INTOROBOT_LORAWAN_RESP_SERVER_ACK = false;
        if(_timeout == 0){
            return LORAMAC_SENDING;
        }
    }else{
        _macSendStatus = LORAMAC_SEND_FAIL;
        return LORAMAC_SEND_FAIL;
    }

    if(_timeout < 120){
        _timeout = 120;
    }
    _macRunStatus = 0;
    uint32_t prevTime = millis();
    while(1){
        if(_macRunStatus == ep_lorawan_mcpsconfirm_confirmed){
            if(_ackReceived == true){
                return LORAMAC_SEND_OK;
            }else{
                _macSendStatus = LORAMAC_SEND_FAIL;
                return LORAMAC_SEND_FAIL;
            }
        }

        if(millis() - prevTime >= _timeout * 1000){
            LoRaMacAbortRun();
            _macSendStatus = LORAMAC_SEND_FAIL;
            return LORAMAC_SEND_FAIL;
        }
        intorobot_process();
    }
    return LORAMAC_SEND_FAIL;
}

int LoRaWanClass::sendUnconfirmed(uint8_t port, uint8_t *buffer, uint16_t len, uint16_t timeout)
{
    McpsReq_t mcpsReq;
    LoRaMacTxInfo_t txInfo;
    uint32_t _timeout = timeout;

    LoRaMacStatus_t loramacStatus = LoRaMacQueryTxPossible( len, &txInfo ) ;

    if(loramacStatus != LORAMAC_STATUS_OK) {
        WLORAWAN_DEBUG("LoRaWan send empty frame\r\n");
        // Send empty frame in order to flush MAC commands
        mcpsReq.Type = MCPS_UNCONFIRMED;
        mcpsReq.Req.Unconfirmed.fBuffer = NULL;
        mcpsReq.Req.Unconfirmed.fBufferSize = 0;
        mcpsReq.Req.Unconfirmed.Datarate = _macDatarate;
    } else {
        // WLORAWAN_DEBUG("LoRaWan send unconfirmed frame\r\n");
        mcpsReq.Type = MCPS_UNCONFIRMED;
        mcpsReq.Req.Unconfirmed.fPort = port;
        mcpsReq.Req.Unconfirmed.fBuffer = buffer;
        mcpsReq.Req.Unconfirmed.fBufferSize = len;
        mcpsReq.Req.Unconfirmed.Datarate = _macDatarate;
    }

    if( LoRaMacMcpsRequest( &mcpsReq ) == LORAMAC_STATUS_OK ) {
        WLORAWAN_DEBUG("LoRaWan send unnconfirm frame status OK!!!\r\n");
        _macSendStatus = LORAMAC_SENDING;
        INTOROBOT_LORAWAN_RESP_SERVER_ACK = false;
        if(_timeout == 0){
            return LORAMAC_SENDING;
        }
    }else{
        _macSendStatus = LORAMAC_SEND_FAIL;
        return LORAMAC_SEND_FAIL;
    }

    if(_timeout < 60){
        _timeout = 60;
    }

    _macRunStatus = 0;
    uint32_t prevTime = millis();
    while(1){
        if(_macRunStatus == ep_lorawan_mcpsconfirm_unconfirmed){
            return LORAMAC_SEND_OK;
        }

        if(millis() - prevTime >= _timeout * 1000){
            LoRaMacAbortRun();
            _macSendStatus = LORAMAC_SEND_FAIL;
            return LORAMAC_SEND_FAIL;
        }
        intorobot_process();
    }
    return LORAMAC_SEND_FAIL;
}

//发送状态查询 1-发送中 0-成功 -1失败
int8_t LoRaWanClass::sendStatus(void)
{
    return _macSendStatus;
}

uint16_t LoRaWanClass::receive(uint8_t *buffer, uint16_t length, int *rssi)
{
    uint16_t size = 0;
    if(_available) {
        _available = false; //数据已读取
        if(length < _bufferSize){
            size = length;
        }else{
            size = _bufferSize;
        }
        *rssi = _macRssi;
        memcpy(buffer, _buffer, size);
        free(_buffer);
        _buffer = NULL;
        return size;
    }
    return 0;
}

void LoRaWanClass::setProtocol(lorawan_protocol_t type)
{
    switch(type){
    case LORAWAN_STANDARD:
        System.disableFeature(SYSTEM_FEATURE_DATA_PROTOCOL_ENABLED);   //关闭datapoint数据处理
        System.enableFeature(SYSTEM_FEATURE_STANDARD_LORAWAN_ENABLED); //运行标准协议
        break;

    case LORAWAN_STANDARD_EXTEND:
        System.enableFeature(SYSTEM_FEATURE_STANDARD_LORAWAN_ENABLED); //运行标准协议
        break;

    case LORAWAN_NONSTANDARD_EXTEND:
        System.disableFeature(SYSTEM_FEATURE_STANDARD_LORAWAN_ENABLED); //不运行标准协议
        break;

    default:
        break;
    }
}

void LoRaWanClass::setOTAAParams(char *devEUI, char *appEUI, char *appKey)
{
    HAL_PARAMS_Set_System_device_id(devEUI);
    HAL_PARAMS_Set_System_appeui(appEUI);
    HAL_PARAMS_Set_System_appkey(appKey);
    HAL_PARAMS_Save_Params();
}

void LoRaWanClass::setABPParams(char *devAddr, char *nwkSKey, char *appSKey)
{
    HAL_PARAMS_Set_System_devaddr(devAddr);
    HAL_PARAMS_Set_System_nwkskey(nwkSKey);
    HAL_PARAMS_Set_System_appskey(appSKey);
    HAL_PARAMS_Save_Params();
}

void LoRaWanClass::getDeviceEUI(char *devEUI, uint16_t len)
{
    char deveui[24]={0};
    HAL_PARAMS_Get_System_device_id(deveui, sizeof(deveui));
    strncpy(devEUI,deveui,len);
}

void LoRaWanClass::getDeviceAddr(char *devAddr, uint16_t len)
{
    char devaddr[16] = {0};
    HAL_PARAMS_Get_System_devaddr(devaddr, sizeof(devaddr));
    strncpy(devAddr,devaddr,len);
}

void LoRaWanClass::getAppEUI(char *appEUI, uint16_t len)
{
    char appeui[24]={0};
    HAL_PARAMS_Get_System_appeui(appeui, sizeof(appeui));
    strncpy(appEUI,appeui,len);
}

void LoRaWanClass::setMacClassType(DeviceClass_t classType)
{
    MibRequestConfirm_t mibReq;
    mibReq.Type = MIB_DEVICE_CLASS;
    mibReq.Param.Class = classType;
    LoRaMacMibSetRequestConfirm( &mibReq );
    _classType = classType;
}

DeviceClass_t LoRaWanClass::getMacClassType(void)
{
    MibRequestConfirm_t mibReq;
    mibReq.Type = MIB_DEVICE_CLASS;
    LoRaMacMibGetRequestConfirm( &mibReq );
    _classType = mibReq.Param.Class;
    return _classType;
}

void LoRaWanClass::setTxPower(uint8_t index)
{
    if(index > 5){
        return;
    }
    MibRequestConfirm_t mibReq;
    mibReq.Type = MIB_CHANNELS_DEFAULT_TX_POWER;
    mibReq.Param.ChannelsDefaultTxPower = index;
    LoRaMacMibSetRequestConfirm( &mibReq );

    mibReq.Type = MIB_CHANNELS_TX_POWER;
    mibReq.Param.ChannelsTxPower = index;
    LoRaMacMibSetRequestConfirm( &mibReq );
}

uint8_t LoRaWanClass::getTxPower(void)
{
    MibRequestConfirm_t mibReq;
    mibReq.Type = MIB_CHANNELS_TX_POWER;
    if(LoRaMacMibGetRequestConfirm( &mibReq ) != LORAMAC_STATUS_OK){
        return;
    }
    _txPower = mibReq.Param.ChannelsTxPower;
    return _txPower;
}

void LoRaWanClass::setDataRate(uint8_t datarate)
{
    if(datarate > DR_5){
        return;
    }
    _macDatarate = datarate;

    MibRequestConfirm_t mibReq;
    mibReq.Type = MIB_CHANNELS_DEFAULT_DATARATE;
    mibReq.Param.ChannelsDefaultDatarate = _macDatarate;
    LoRaMacMibSetRequestConfirm( &mibReq );

    mibReq.Type = MIB_CHANNELS_DATARATE;
    mibReq.Param.ChannelsDatarate = _macDatarate;
    LoRaMacMibSetRequestConfirm( &mibReq );
}

uint8_t LoRaWanClass::getDataRate(void)
{
    return _macDatarate;
}

void LoRaWanClass::setAdrOn(bool AdrOn)
{
    MibRequestConfirm_t mibReq;
    _adrOn = AdrOn;
    mibReq.Type = MIB_ADR;
    mibReq.Param.AdrEnable = _adrOn;
    LoRaMacMibSetRequestConfirm( &mibReq );
}

bool LoRaWanClass::getAdrOn(void)
{
    return _adrOn;
}

uint16_t LoRaWanClass::getDutyCyclePrescaler(void)
{
    return LoRaMacGetDutyCycle();
}

void LoRaWanClass::setDutyCyclePrescaler(uint16_t dutyCycle)
{
    LoRaMacSetDutyCycle(dutyCycle);
}

void LoRaWanClass::setChannelFreq(uint8_t channel, uint32_t freq)
{
    if((channel > 15) || (freq > 525000000) || (freq < 137000000)){
        return;
    }
    ChannelParams_t   channelParams = {freq, 0, { ( ( DR_5 << 4 ) | DR_0 ) }, 0};
    LoRaMacChannelAdd(channel,channelParams);
}

uint32_t LoRaWanClass::getChannelFreq(uint8_t channel)
{
    if(channel > 15){
        return;
    }
    return LoRaMacGetChannelFreq(channel);
}

void LoRaWanClass::setChannelDRRange(uint8_t channel, uint8_t minDR, uint8_t maxDR)
{
    if(channel > 15 || maxDR > DR_5){
        return;
    }
    if(minDR > maxDR){
        return;
    }
    uint32_t tmpFreq = LoRaMacGetChannelFreq(channel);
    ChannelParams_t channelParams = {tmpFreq, 0, { ( ( maxDR << 4 ) | minDR ) }, 0};
    LoRaMacChannelAdd(channel,channelParams);
}

bool LoRaWanClass::getChannelDRRange(uint8_t channel, uint8_t *minDR,uint8_t *maxDR)
{
    if(channel > 15){
        return false;
    }else{
        LoRaMacGetChannelDRRange(channel,minDR,maxDR);
        return true;
    }
}

void LoRaWanClass::setChannelStatus(uint8_t channel, bool enable)
{
    if(channel > 15){
        return;
    }

    MibRequestConfirm_t mibReq;
    uint16_t channelMask = 0;
    mibReq.Type = MIB_CHANNELS_MASK;
    if(LoRaMacMibGetRequestConfirm( &mibReq ) != LORAMAC_STATUS_OK){
        WLORAWAN_DEBUG("get channelMask fail\r\n");
        return;
    }

    channelMask = *mibReq.Param.ChannelsMask;
    WLORAWAN_DEBUG("channelMask1=%d\r\n",channelMask);
    if(enable){
        channelMask = channelMask | (1<<channel);
    }else{
        channelMask = channelMask & (~(1<<channel));
    }

    WLORAWAN_DEBUG("channelMask2=%d\r\n",channelMask);
    mibReq.Type = MIB_CHANNELS_DEFAULT_MASK;
    mibReq.Param.ChannelsDefaultMask = &channelMask;
    LoRaMacMibSetRequestConfirm( &mibReq );

    mibReq.Type = MIB_CHANNELS_MASK;
    mibReq.Param.ChannelsMask = &channelMask;
    LoRaMacMibSetRequestConfirm( &mibReq );
}

bool LoRaWanClass::getChannelStatus(uint8_t channel)
{
    if(channel > 15){
        return;
    }

    MibRequestConfirm_t mibReq;
    uint16_t channelMask = 0;
    mibReq.Type = MIB_CHANNELS_MASK;
    if(LoRaMacMibGetRequestConfirm( &mibReq ) != LORAMAC_STATUS_OK){
        return;
    }
    channelMask = *mibReq.Param.ChannelsMask;
    if(channelMask & (1<<channel)){
        return true;
    }else{
        return false;
    }
}

void LoRaWanClass::setConfirmedNbTrials(uint8_t trials)
{
    _confirmedNbTrials = trials;
}

uint8_t LoRaWanClass::getConfirmedNbTrials(void)
{
    return _confirmedNbTrials;
}

void LoRaWanClass::setUnconfirmedNbTrials(uint8_t trials)
{
    MibRequestConfirm_t mibReq;

    _unConfirmedNbTrials = trials;
    if(trials >= 1 && trials <= 15) {
        mibReq.Type = MIB_CHANNELS_NB_REP;
        mibReq.Param.ChannelNbRep = trials;
        LoRaMacMibSetRequestConfirm( &mibReq );
    }
}

uint8_t LoRaWanClass::getUnconfirmedNbTrials(void)
{
    return _unConfirmedNbTrials;
}

void LoRaWanClass::setJoinNbTrials(uint8_t trials)
{
    _joinNbTrials = trials;
}

uint8_t LoRaWanClass::getJoinNbTrials(void)
{
    return _joinNbTrials;
}

void LoRaWanClass::setUpCounter(uint32_t counter)
{
    MibRequestConfirm_t mibReq;
    mibReq.Type = MIB_UPLINK_COUNTER;
    mibReq.Param.UpLinkCounter = counter;
    LoRaMacMibSetRequestConfirm(&mibReq);
}

uint32_t LoRaWanClass::getUpCounter(void)
{
    MibRequestConfirm_t mibReq;
    LoRaMacStatus_t status;

    mibReq.Type = MIB_UPLINK_COUNTER;
    status = LoRaMacMibGetRequestConfirm( &mibReq );
    if(status == LORAMAC_STATUS_OK) {
        return mibReq.Param.UpLinkCounter;
    }
}

void LoRaWanClass::setDownCounter(uint32_t counter)
{
    MibRequestConfirm_t mibReq;
    mibReq.Type = MIB_DOWNLINK_COUNTER;
    mibReq.Param.DownLinkCounter = counter;
    LoRaMacMibSetRequestConfirm(&mibReq);
}

uint32_t LoRaWanClass::getDownCounter(void)
{
    MibRequestConfirm_t mibReq;
    LoRaMacStatus_t status;

    mibReq.Type = MIB_DOWNLINK_COUNTER;
    status = LoRaMacMibGetRequestConfirm( &mibReq );
    if(status == LORAMAC_STATUS_OK) {
        return mibReq.Param.DownLinkCounter;
    }
}

void LoRaWanClass::setRX2Params(uint8_t datarate, uint32_t frequency)
{
    if(datarate > DR_5){
        return 0;
    }
    MibRequestConfirm_t mibReq;
    mibReq.Type = MIB_RX2_DEFAULT_CHANNEL;
    mibReq.Param.Rx2DefaultChannel = ( Rx2ChannelParams_t ){ frequency,datarate };
    LoRaMacMibSetRequestConfirm( &mibReq );

    mibReq.Type = MIB_RX2_CHANNEL;
    mibReq.Param.Rx2DefaultChannel = ( Rx2ChannelParams_t ){ frequency,datarate };
    LoRaMacMibSetRequestConfirm( &mibReq );
}

void LoRaWanClass::getRX2Params(uint8_t &datarate, uint32_t &frequency)
{
    MibRequestConfirm_t mibReq;
    LoRaMacStatus_t status;
    Rx2ChannelParams_t rx2Params;
    mibReq.Type = MIB_RX2_CHANNEL;
    status = LoRaMacMibGetRequestConfirm( &mibReq );
    if(status == LORAMAC_STATUS_OK){
        rx2Params = mibReq.Param.Rx2Channel;
        datarate = rx2Params.Datarate;
        frequency = rx2Params.Frequency;
    }
}

void LoRaWanClass::setRX1Delay(uint16_t delay)
{
    MibRequestConfirm_t mibReq;
    mibReq.Type = MIB_RECEIVE_DELAY_1;
    mibReq.Param.ReceiveDelay1 = delay;
    LoRaMacMibSetRequestConfirm( &mibReq );
}

uint16_t LoRaWanClass::getRX1Delay(void)
{
    MibRequestConfirm_t mibReq;
    LoRaMacStatus_t status;
    mibReq.Type = MIB_RECEIVE_DELAY_1;
    status = LoRaMacMibGetRequestConfirm( &mibReq );
    if(status == LORAMAC_STATUS_OK){
        return mibReq.Param.ReceiveDelay1;
    }else{
        return 0;
    }
}

uint8_t LoRaWanClass::getMargin(void)
{
    return _demodMargin;
}

uint8_t LoRaWanClass::getGatewayNumber(void)
{
    return _nbGateways;
}

uint8_t LoRaWanClass::getSnr(void)
{
    return _macSnr;
}

int16_t LoRaWanClass::getRssi(void)
{
    return _macRssi;
}

uint16_t LoRaWanClass::getBatteryVoltage(uint16_t pin)
{
    return BoardBatteryMeasureVolage(pin);
}

//P2P透传接口
int LoRaClass::radioSend(uint8_t *buffer, uint16_t length, uint32_t timeout)
{
    uint32_t _timeout = 0;
    if(timeout == 0){
        _timeout = _txTimeout;
    }else{
        _timeout = timeout;
    }

    if(_timeout < 3000){
        _timeout = 3000;
    }

    Radio.SetTxConfig(_modem, _power, _fdev, _bandwidth, _datarate, _coderate, _preambleLen, _fixLenOn, _crcOn, _freqHopOn, _hopPeriod, _iqInverted, _timeout);
    Radio.Send(buffer, length);

    if(timeout == 0){
        _radioSendStatus = 1;
        return 1;
    }

    _radioRunStatus = 0;
    uint32_t prevTime = millis();
    while(1){
        if(_radioRunStatus == ep_lora_radio_tx_done){
            _radioSendStatus = 0;
            return 0;
        }
        if(_radioRunStatus == ep_lora_radio_tx_fail){
            _radioSendStatus = -1;
            return -1;
        }

        if(millis() - prevTime >= _timeout){
            _radioSendStatus = -1;
            return -1;
        }
        intorobot_process();
    }
}

void LoRaClass::radioStartRx(uint32_t timeout)
{
    Radio.SetRxConfig(_modem, _bandwidth, _datarate, _coderate, _bandwidthAfc, _preambleLen, _symbTimeout, _fixLenOn, _payloadLen, _crcOn, _freqHopOn, _hopPeriod, _iqInverted, _rxContinuous);
    Radio.Rx(timeout);
}

int8_t LoRaClass::radioSendStatus(void)
{
    return _radioSendStatus;
}

uint16_t LoRaClass::radioRx(uint8_t *buffer, uint16_t length, int16_t *rssi)
{
    uint16_t size = 0;
    if(_available){
        _available = false;
        if(length < _bufferSize){
            size = length;
        }else{
            size = _bufferSize;
        }
        *rssi = _rssi;
        memcpy( buffer, _buffer, size);
        free(_buffer);
        _buffer = NULL;
        return size;
    }else{
        return 0;
    }
}

bool LoRaClass::radioCad(void)
{
    _radioRunStatus = 0;
    uint32_t prevTime = millis();
    Radio.StartCad();
    while(1){
        if(_radioRunStatus == ep_lora_radio_cad_detected){
            return true;
        }else if(_radioRunStatus == ep_lora_radio_cad_done){
            return false;
        }
        if(millis() - prevTime >= 1000){
            return false;
        }
        intorobot_process();
    }
}

int16_t LoRaClass::radioGetRssi(void)
{
    return _rssi;
}

uint8_t LoRaClass::radioGetSnr(void)
{
    return _snr;
}

void LoRaClass::radioSetSleep(void)
{
    Radio.Sleep();
}

void LoRaClass::radioSetFreq(uint32_t freq)
{
    if((freq > 525000000) || (freq < 137000000)){
        return;
    }
    _freq = freq;
    Radio.SetChannel(_freq);
}

uint32_t LoRaClass::radioGetFreq(void)
{
    return _freq;
}

void LoRaClass::radioSetMaxPayloadLength(uint8_t maxPayloadLen)
{
    _maxPayloadLen = maxPayloadLen;
    Radio.SetMaxPayloadLength(_modem, _maxPayloadLen);
}

uint8_t LoRaClass::radioGetMaxPayloadLength(void)
{
    return _maxPayloadLen;
}

void LoRaClass::radioSetModem(RadioModems_t modem)
{
    _modem = modem;
    Radio.SetModem(_modem);
}

RadioModems_t LoRaClass::radioGetModem(void)
{
    return _modem;
}

void LoRaClass::radioSetBandwidth(uint8_t bandwidth)
{
    _bandwidth = bandwidth;
}

uint8_t LoRaClass::radioGetBandwidth(void)
{
    return _bandwidth;
}

void LoRaClass::radioSetSF(uint8_t sf)
{
    _datarate = sf;
}

uint8_t LoRaClass::radioGetSF(void)
{
    return _datarate;
}

void LoRaClass::radioSetCR(uint8_t coderate)
{
    _coderate = coderate;
}

uint8_t LoRaClass::radioGetCR(void)
{
    return _coderate;
}

void LoRaClass::radioSetPreambleLen(uint16_t preambleLen)
{
    _preambleLen = preambleLen;
}

uint16_t LoRaClass::radioGetPreambleLen(void)
{
    return _preambleLen;
}

void LoRaClass::radioSetFixLenOn(bool fixLenOn)
{
    _fixLenOn = fixLenOn;
}

bool LoRaClass::radioGetFixLenOn(void)
{
    return _fixLenOn;
}

void LoRaClass::radioSetCrcOn(bool crcOn)
{
    _crcOn = crcOn;
}

bool LoRaClass::radioGetCrcOn(void)
{
    return _crcOn;
}

void LoRaClass::radioSetFreqHopOn(bool FreqHopOn)
{
    _freqHopOn = FreqHopOn;
}

bool LoRaClass::radioGetFreqHopOn(void)
{
    return _freqHopOn;
}

void LoRaClass::radioSetHopPeriod(uint8_t hopPeriod)
{
    _hopPeriod = hopPeriod;
}

uint8_t LoRaClass::radioGetHopPeriod(void)
{
    return _hopPeriod;
}

void LoRaClass::radioSetIqInverted(bool iqInverted)
{
    _iqInverted = iqInverted;
}

bool LoRaClass::radioGetIqInverted(void)
{
    return _iqInverted;
}

void LoRaClass::radioSetRxContinuous(bool rxContinuous)
{
    _rxContinuous = rxContinuous;
}

bool LoRaClass::radioGetRxContinuous(void)
{
    return _rxContinuous;
}

//发送射频相关参数
void LoRaClass::radioSetTxPower(int8_t txPower)
{
    _power = txPower;
}

int8_t LoRaClass::radioGetTxPower(void)
{
    return _power;
}

//接收射频相关参数
void LoRaClass::radioSetFixPayloadLen(uint8_t payloadLen)
{
    _payloadLen = payloadLen;
}

uint8_t LoRaClass::radioGetFixPayloadLen(void)
{
    return _payloadLen;
}

void LoRaClass::radioSetSymbTimeout(uint16_t symbTimeout)
{
    if(symbTimeout >= 4 && symbTimeout <= 1023) {
        _symbTimeout = symbTimeout;
    }
}

uint16_t LoRaClass::radioGetSymbTimeout(void)
{
    return _symbTimeout;
}

uint8_t LoRaClass::radioReadReg(uint8_t addr)
{
    return Radio.Read(addr);
}

void LoRaClass::radioWriteReg(uint8_t addr, uint8_t data)
{
    Radio.Write(addr,data);
}

#endif
