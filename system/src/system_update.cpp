/**
 ******************************************************************************
 Copyright (c) 2013-2015 IntoRobot Industries, Inc.  All rights reserved.

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
#include "stream_string.h"
#include "system_update.h"
#include "system_cloud.h"
#include "core_hal.h"
#include "updater_hal.h"
#include "params_hal.h"
#include "wiring_ticks.h"
#include "wiring_system.h"

const static char *TAG = "sys-update";

#ifdef START_DFU_FLASHER_SERIAL_SPEED
static uint32_t start_dfu_flasher_serial_speed = START_DFU_FLASHER_SERIAL_SPEED;
#endif
#ifdef START_YMODEM_FLASHER_SERIAL_SPEED
static uint32_t start_ymodem_flasher_serial_speed = START_YMODEM_FLASHER_SERIAL_SPEED;
#endif

void set_start_dfu_flasher_serial_speed(uint32_t speed) {
#ifdef START_DFU_FLASHER_SERIAL_SPEED
    start_dfu_flasher_serial_speed = speed;
#endif
}

void set_start_ymodem_flasher_serial_speed(uint32_t speed) {
#ifdef START_YMODEM_FLASHER_SERIAL_SPEED
    start_ymodem_flasher_serial_speed = speed;
#endif
}

void system_lineCodingBitRateHandler(uint32_t bitrate) {
#ifdef START_DFU_FLASHER_SERIAL_SPEED
    if (bitrate == start_dfu_flasher_serial_speed) {
        HAL_Core_Enter_DFU_Mode(false);
    }
#endif
}

#ifndef configNO_NETWORK

UpdaterClass::UpdaterClass()
    : _error(0)
    , _buffer(0)
    , _bufferLen(0)
    , _size(0)
    , _mode(UPDATER_MODE_UPDATE)
    , _startAddress(0)
    , _currentAddress(0)
    , _progressCb(NULL)
{
}

void UpdaterClass::_reset() {
    if (_buffer)
        delete[] _buffer;
    _buffer = 0;
    _bufferLen = 0;
    _startAddress = 0;
    _currentAddress = 0;
    _size = 0;
    _mode = UPDATER_MODE_UPDATE;
}

bool UpdaterClass::begin(size_t size, updater_mode_t mode) {
    if(_size > 0) {
        MOLMC_LOGD(TAG, "[begin] already running");
        return false;
    }

    if(size == 0) {
        _setError(UPDATE_ERROR_SIZE);
        return false;
    }

    _reset();
    clearError(); //  _error = 0

    uint32_t updateStartAddress = 0;
    if(UPDATER_MODE_UPDATE == mode) {
        updateStartAddress = HAL_Update_StartAddress();
        uint32_t currentSketchSize = HAL_Update_FlashLength();
        //initialize
        if (updateStartAddress) {
            MOLMC_LOGD(TAG, "[begin] updateStartAddress:  0x%08X (%d)", updateStartAddress, updateStartAddress);
            MOLMC_LOGD(TAG, "[begin] updateFileSize:      0x%08X (%d)", size, size);

            if(size >= currentSketchSize) {
                _setError(UPDATE_ERROR_SPACE);
                return false;
            }
        } else {
            MOLMC_LOGD(TAG, "[begin] updateStartAddress = 0.");
            return false;
        }
    }

    //initialize
    _startAddress = updateStartAddress;
    _currentAddress = _startAddress;
    _size = size;
    _mode = mode;

    if (System.freeMemory() > 2 * UPDATE_SECTOR_SIZE) {
        _bufferSize = UPDATE_SECTOR_SIZE;
    } else {
        _bufferSize = 256;
    }
    _buffer = new uint8_t[_bufferSize];

    MOLMC_LOGD(TAG, "[begin] _startAddress:       0x%08X (%d)", _startAddress, _startAddress);
    MOLMC_LOGD(TAG, "[begin] _currentAddress:     0x%08X (%d)", _currentAddress, _currentAddress);
    MOLMC_LOGD(TAG, "[begin] _size:               0x%08X (%d)", _size, _size);

    _md5.begin();
    return true;
}

bool UpdaterClass::setMD5(const char * expected_md5){
    if(strlen(expected_md5) != 32) {
        return false;
    }
    _target_md5 = expected_md5;
    return true;
}

void UpdaterClass::onProgress(THandlerFunction_Progress fn){
    if(NULL != fn) {
        _progressCb = fn;
    }
}

bool UpdaterClass::end(bool evenIfRemaining){
    if(_size == 0){
        MOLMC_LOGD(TAG, "no update");
        return false;
    }

    if(hasError() || (!isFinished() && !evenIfRemaining)){
        MOLMC_LOGD(TAG, "premature end: res:%u, pos:%u/%u", getError(), progress(), _size);
        _reset();
        return false;
    }

    if(evenIfRemaining) {
        if(_bufferLen > 0) {
            _writeBuffer();
        }
        _size = progress();
    }

    _md5.calculate();
    if(_target_md5.length()) {
        if(_target_md5 != _md5.toString()) {
            _setError(UPDATE_ERROR_MD5);
            _reset();
            return false;
        } else {
            MOLMC_LOGD(TAG, "MD5 Success: %s", _target_md5.c_str());
        }
    }

    MOLMC_LOGD(TAG, "Staged: address:0x%08X, size:0x%08X", _startAddress, _size);

    if(UPDATER_MODE_UPDATE == _mode) {
        HAL_Set_Update_Flag(_size);
    }
    _reset();
    return true;
}

bool UpdaterClass::_writeBuffer() {
    if(NULL != _progressCb) {
        _progressCb(_buffer, _bufferLen, _currentAddress - _startAddress, _size);
    }

    if(UPDATER_MODE_UPDATE == _mode) {
        int result = HAL_Update_Flash(_buffer, _currentAddress, _bufferLen, NULL);
        if (result) {
            _currentAddress = (_startAddress + _size);
            _setError(UPDATE_ERROR_WRITE);
            return false;
        }
    }

    _md5.add(_buffer, _bufferLen);
    _currentAddress += _bufferLen;
    _bufferLen = 0;
    return true;
}

size_t UpdaterClass::write(uint8_t *data, size_t len) {
    if(hasError() || !isRunning())
        return 0;

    if(len > remaining()){
        //fail instead
        _setError(UPDATE_ERROR_SPACE);
        return 0;
    }

    size_t left = len;

    while((_bufferLen + left) > _bufferSize) {
        size_t toBuff = _bufferSize - _bufferLen;
        memcpy(_buffer + _bufferLen, data + (len - left), toBuff);
        _bufferLen += toBuff;
        if(!_writeBuffer()){
            return len - left;
        }
        left -= toBuff;
        intorobot_cloud_handle();
    }
    //lets see whats left
    memcpy(_buffer + _bufferLen, data + (len - left), left);
    _bufferLen += left;
    if(_bufferLen == remaining()){
        //we are at the end of the update, so should write what's left to flash
        if(!_writeBuffer()){
            return len - left;
        }
    }
    return len;
}

size_t UpdaterClass::writeStream(Stream &data) {
    size_t written = 0;
    size_t toRead = 0;

    if(hasError() || !isRunning())
        return 0;

    while(remaining()) {
        if((_bufferLen + remaining()) <= UPDATE_SECTOR_SIZE) {
            toRead = data.readBytes((char *)_buffer + _bufferLen, remaining());
        } else {
            toRead = data.readBytes((char *)_buffer + _bufferLen, (_bufferSize - _bufferLen));
        }

        if(toRead == 0) { //Timeout
            delay(100);
            toRead = data.readBytes(_buffer + _bufferLen, (_bufferSize - _bufferLen));
            if(toRead == 0) { //Timeout
                _currentAddress = (_startAddress + _size);
                _setError(UPDATE_ERROR_STREAM);
                _reset();
                return written;
            }
        }
        _bufferLen += toRead;
        if((_bufferLen == remaining() || _bufferLen == _bufferSize) && !_writeBuffer())
            return written;
        written += toRead;
        intorobot_cloud_handle();
        MOLMC_LOGD(TAG, "writeStream toRead = %d , _bufferLen = %d, written = %d", toRead, _bufferLen, written);
    }
    return written;
}

void UpdaterClass::_setError(int error){
    StreamString errorString;

    _error = error;
    Update.printError(errorString);

    MOLMC_LOGD(TAG, "%s", errorString.c_str());
}

void UpdaterClass::printError(Print &out){
    out.printf("ERROR[%u]: ", _error);
    if(_error == UPDATE_ERROR_OK){
        out.println("No Error");
    } else if(_error == UPDATE_ERROR_WRITE){
        out.println("Flash Write Failed");
    } else if(_error == UPDATE_ERROR_ERASE){
        out.println("Flash Erase Failed");
    } else if(_error == UPDATE_ERROR_READ){
        out.println("Flash Read Failed");
    } else if(_error == UPDATE_ERROR_SPACE){
        out.println("Not Enough Space");
    } else if(_error == UPDATE_ERROR_SIZE){
        out.println("Bad Size Given");
    } else if(_error == UPDATE_ERROR_STREAM){
        out.println("Stream Read Timeout");
    } else if(_error == UPDATE_ERROR_MD5){
        out.println("MD5 Check Failed");
    } else {
        out.println("UNKNOWN");
    }
}

UpdaterClass Update;

#endif
