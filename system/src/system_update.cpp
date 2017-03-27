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
#include "ota_flash_hal.h"
#include "params_hal.h"
#include "wiring_ticks.h"

/*debug switch*/
#define SYSTEM_UPDATE_DEBUG

#ifdef SYSTEM_UPDATE_DEBUG
#define SUPDATE_DEBUG(...)  do {DEBUG(__VA_ARGS__);}while(0)
#define SUPDATE_DEBUG_D(...)  do {DEBUG_D(__VA_ARGS__);}while(0)
#else
#define SUPDATE_DEBUG(...)
#define SUPDATE_DEBUG_D(...)
#endif

#ifdef START_DFU_FLASHER_SERIAL_SPEED
static uint32_t start_dfu_flasher_serial_speed = START_DFU_FLASHER_SERIAL_SPEED;
#endif
#ifdef START_YMODEM_FLASHER_SERIAL_SPEED
static uint32_t start_ymodem_flasher_serial_speed = START_YMODEM_FLASHER_SERIAL_SPEED;
#endif

void set_start_dfu_flasher_serial_speed(uint32_t speed)
{
#ifdef START_DFU_FLASHER_SERIAL_SPEED
    start_dfu_flasher_serial_speed = speed;
#endif
}

void set_start_ymodem_flasher_serial_speed(uint32_t speed)
{
#ifdef START_YMODEM_FLASHER_SERIAL_SPEED
    start_ymodem_flasher_serial_speed = speed;
#endif
}

void system_lineCodingBitRateHandler(uint32_t bitrate)
{
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
    , _startAddress(0)
    , _currentAddress(0)
    , _progress(0)
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
    _progress = 0;
}

bool UpdaterClass::begin(size_t size, uint32_t startAddress, uint32_t maxSize) {
    StreamString error;

    if(_size > 0){
        SUPDATE_DEBUG("[begin] already running");
        return false;
    }

    if(size == 0) {
        _error = UPDATE_ERROR_SIZE;
        printError(error);
        error.trim(); // remove line ending
        SUPDATE_DEBUG("error : (%s)", error.c_str());
        return false;
    }

    _reset();
    _error = 0;

    if (startAddress) {

        SUPDATE_DEBUG("[begin] updateStartAddress:  0x%08X (%d)", _startAddress, _startAddress);
        SUPDATE_DEBUG("[begin] updateFileSize:      0x%08X (%d)", size, size);

        if(size >= maxSize) {
            _error = UPDATE_ERROR_SPACE;
            printError(error);
            error.trim(); // remove line ending
            SUPDATE_DEBUG("error : (%s)", error.c_str());
            return false;
        }
    } else {
        SUPDATE_DEBUG("[begin] startAddress = 0.");
        return false;
    }

    //initialize
    _startAddress = startAddress;
    _currentAddress = _startAddress;
    _size = size;
    _buffer = new uint8_t[UPDATE_SECTOR_SIZE];

    SUPDATE_DEBUG("[begin] _startAddress:       0x%08X (%d)", _startAddress, _startAddress);
    SUPDATE_DEBUG("[begin] _currentAddress:     0x%08X (%d)", _currentAddress, _currentAddress);
    SUPDATE_DEBUG("[begin] _size:               0x%08X (%d)", _size, _size);

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

bool UpdaterClass::setSendProgressCb(progressCb Cb){
    if(NULL == Cb) {
        return false;
    }
    _progressCb = Cb;
    return true;
}

bool UpdaterClass::end(bool evenIfRemaining){
    if(_size == 0){
        SUPDATE_DEBUG("no update");
        return false;
    }

    if(hasError() || (!isFinished() && !evenIfRemaining)){
        SUPDATE_DEBUG("premature end: res:%u, pos:%u/%u", getError(), progress(), _size);
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
        if(_target_md5 != _md5.toString()){
            _error = UPDATE_ERROR_MD5;
            SUPDATE_DEBUG("MD5 Failed: expected:%s, calculated:%s", _target_md5.c_str(), _md5.toString().c_str());
            _reset();
            return false;
        } else {
            SUPDATE_DEBUG("MD5 Success: %s", _target_md5.c_str());
        }
    }

    SUPDATE_DEBUG("Staged: address:0x%08X, size:0x%08X", _startAddress, _size);

    _reset();
    return true;
}

bool UpdaterClass::_writeBuffer(){
    StreamString error;

    int result = HAL_FLASH_Update(_buffer, _currentAddress, _bufferLen, NULL);
    if (result) {
        _error = UPDATE_ERROR_WRITE;
        _currentAddress = (_startAddress + _size);
        printError(error);
        error.trim(); // remove line ending
        SUPDATE_DEBUG("error : (%s)", error.c_str());
        return false;
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
        _error = UPDATE_ERROR_SPACE;
        return 0;
    }

    size_t left = len;

    while((_bufferLen + left) > UPDATE_SECTOR_SIZE) {
        size_t toBuff = UPDATE_SECTOR_SIZE - _bufferLen;
        memcpy(_buffer + _bufferLen, data + (len - left), toBuff);
        _bufferLen += toBuff;
        if(!_writeBuffer()){
            return len - left;
        }
        left -= toBuff;
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
    StreamString error;
    size_t written = 0;
    size_t toRead = 0;
    uint8_t progress = 0;

    if(hasError() || !isRunning())
        return 0;

    if(NULL != _progressCb) {
        _progressCb(1);
    }
    while(remaining()) {
        if((_bufferLen + remaining()) <= UPDATE_SECTOR_SIZE) {
            toRead = data.readBytes(_buffer + _bufferLen, remaining());
        } else {
            toRead = data.readBytes(_buffer + _bufferLen, (UPDATE_SECTOR_SIZE - _bufferLen));
        }

        if(toRead == 0) { //Timeout
            delay(100);
            toRead = data.readBytes(_buffer + _bufferLen, (UPDATE_SECTOR_SIZE - _bufferLen));
            if(toRead == 0) { //Timeout
                _error = UPDATE_ERROR_STREAM;
                _currentAddress = (_startAddress + _size);
                printError(error);
                error.trim(); // remove line ending
                SUPDATE_DEBUG("error : (%s)", error.c_str());
                _reset();
                return written;
            }
        }
        _bufferLen += toRead;
        if((_bufferLen == remaining() || _bufferLen == UPDATE_SECTOR_SIZE) && !_writeBuffer())
            return written;
        written += toRead;
        if(NULL != _progressCb) {
            progress = (written*100)/size();
            if((progress - _progress) > 20) {
                _progressCb(progress);
                _progress = progress;
            }
        }
        intorobot_cloud_handle();
        SUPDATE_DEBUG("writeStream toRead = %d , _bufferLen = %d, written = %d", toRead, _bufferLen, written);
    }
    return written;
}

void UpdaterClass::printError(Stream &out){
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


HTTPUpdate::HTTPUpdate(void)
    : _startAddress(0)
    , _maxSize(0)
    , _size(0)
    , _progressCb(NULL)
{
}

HTTPUpdate::~HTTPUpdate(void)
{
}

HTTPUpdateResult HTTPUpdate::update(const String& url, const String& currentVersion)
{
    HTTPClient http;
    http.begin(url);
    return handleUpdate(http, currentVersion);
}

HTTPUpdateResult HTTPUpdate::update(const String& host, uint16_t port, const String& uri,
        const String& currentVersion)
{
    HTTPClient http;
    http.begin(host, port, uri);
    return handleUpdate(http, currentVersion);
}

size_t HTTPUpdate::size()
{
    return _size;
}

/**
 * return error code as int
 * @return int error code
 */
int HTTPUpdate::getLastError(void)
{
    return _lastError;
}

/**
 * return error code as String
 * @return String error
 */
String HTTPUpdate::getLastErrorString(void)
{
    if(_lastError == 0) {
        return String(); // no error
    }

    // error from Update class
    if(_lastError > 0) {
        StreamString error;
        Update.printError(error);
        error.trim(); // remove line ending
        return String(F("Update error: ")) + error;
    }

    // error from http client
    if(_lastError > -100) {
        return String(F("HTTP error: ")) + HTTPClient::errorToString(_lastError);
    }

    switch(_lastError) {
        case HTTP_UE_TOO_LESS_SPACE:
            return F("To less space");
        case HTTP_UE_SERVER_NOT_REPORT_SIZE:
            return F("Server not Report Size");
        case HTTP_UE_SERVER_FILE_NOT_FOUND:
            return F("File not Found (404)");
        case HTTP_UE_SERVER_FORBIDDEN:
            return F("Forbidden (403)");
        case HTTP_UE_SERVER_WRONG_HTTP_CODE:
            return F("Wrong HTTP code");
        case HTTP_UE_SERVER_FAULTY_MD5:
            return F("Faulty MD5");
        case HTTP_UE_BIN_VERIFY_HEADER_FAILED:
            return F("Verify bin header failed");
        case HTTP_UE_BIN_FOR_WRONG_FLASH:
            return F("bin for wrong flash size");
    }

    return String();
}

bool HTTPUpdate::setStoreStartAddress(uint32_t address){
    _startAddress = address;
    return true;
}

bool HTTPUpdate::setStoreMaxSize(uint32_t size){
    _maxSize = size;
    return true;
}

bool HTTPUpdate::setSendProgressCb(progressCb Cb){
    if(NULL == Cb) {
        return false;
    }
    _progressCb = Cb;
    return true;
}


/**
 *
 * @param http HTTPClient *
 * @param currentVersion const char *
 * @return HTTPUpdateResult
 */
HTTPUpdateResult HTTPUpdate::handleUpdate(HTTPClient& http, const String& currentVersion)
{
    HTTPUpdateResult ret = HTTP_UPDATE_FAILED;

    // use HTTP/1.0 for update since the update handler not support any transfer Encoding
    http.useHTTP10(true);
    http.setReuse(true); /// keep-alive
    http.setTimeout(4000);
    http.setUserAgent(F("Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36"));
    http.addHeader(F("Cache-Control"), F("no-cache"));
    http.addHeader(F("Accept"), F("*/*"));
    http.addHeader(F("Accept-Encoding"), F("gzip,deflate"));
    http.addHeader(F("Accept-Language"), F("zh-CN,eb-US;q=0.8"));

    /*
    if(currentVersion && currentVersion[0] != 0x00) {
        http.addHeader(F("x-version"), currentVersion);
    }
    */

    const char * headerkeys[] = { "X-Md5" };
    size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);

    // track these headers
    http.collectHeaders(headerkeys, headerkeyssize);

    int code = http.GET();
    int len = http.getSize();

    if(code <= 0) {
        SUPDATE_DEBUG("[httpUpdate] HTTP error: %s", http.errorToString(code).c_str());
        _lastError = code;
        http.end();
        return HTTP_UPDATE_FAILED;
    }

    SUPDATE_DEBUG("[httpUpdate] Header read fin.");
    SUPDATE_DEBUG("[httpUpdate] Server header:");
    SUPDATE_DEBUG("[httpUpdate] - code: %d", code);
    SUPDATE_DEBUG("[httpUpdate] - len: %d", len);

    if(http.hasHeader("X-Md5")) {
        SUPDATE_DEBUG("[httpUpdate] - MD5: %s", http.header("X-Md5").c_str());
    }

    /*
    if(currentVersion && currentVersion[0] != 0x00) {
        SUPDATE_DEBUG("[httpUpdate]  - current version: %s", currentVersion.c_str() );
    }
    */

    switch(code) {
        case HTTP_CODE_OK:  ///< OK (Start Update)
            if(len > 0) {
                bool startUpdate = true;
                if(len > (int) HAL_OTA_FlashLength()) {
                    SUPDATE_DEBUG("[httpUpdate] FreeSketchSpace to low (%d) needed: %d", HAL_OTA_FlashLength(), len);
                    startUpdate = false;
                }

                if(!startUpdate) {
                    _lastError = HTTP_UE_TOO_LESS_SPACE;
                    ret = HTTP_UPDATE_FAILED;
                } else {
                    TCPClient * tcp = http.getStreamPtr();

                    //UDP::stopAll();
                    //TCPClient::stopAllExcept(tcp);

                    delay(100);

                    SUPDATE_DEBUG("[httpUpdate] runUpdate flash...");

                    if(runUpdate(*tcp, len, http.header("X-Md5"))) {
                        ret = HTTP_UPDATE_OK;
                        SUPDATE_DEBUG("[httpUpdate] Update ok");
                        http.end();
                    } else {
                        ret = HTTP_UPDATE_FAILED;
                        SUPDATE_DEBUG("[httpUpdate] Update failed");
                    }
                }
            } else {
                _lastError = HTTP_UE_SERVER_NOT_REPORT_SIZE;
                ret = HTTP_UPDATE_FAILED;
                SUPDATE_DEBUG("[httpUpdate] Content-Length is 0 or not set by Server?!");
            }
            break;
        case HTTP_CODE_NOT_MODIFIED:
            ///< Not Modified (No updates)
            ret = HTTP_UPDATE_NO_UPDATES;
            break;
        case HTTP_CODE_NOT_FOUND:
            _lastError = HTTP_UE_SERVER_FILE_NOT_FOUND;
            ret = HTTP_UPDATE_FAILED;
            break;
        case HTTP_CODE_FORBIDDEN:
            _lastError = HTTP_UE_SERVER_FORBIDDEN;
            ret = HTTP_UPDATE_FAILED;
            break;
        default:
            _lastError = HTTP_UE_SERVER_WRONG_HTTP_CODE;
            ret = HTTP_UPDATE_FAILED;
            SUPDATE_DEBUG("[httpUpdate] HTTP Code is (%d)", code);
            break;
    }

    http.end();
    return ret;
}

/**
 * write Update to flash
 * @param in Stream&
 * @param size uint32_t
 * @param md5 String
 * @return true if Update ok
 */
bool HTTPUpdate::runUpdate(Stream& in, uint32_t size, String md5)
{
    StreamString error;

    if(!Update.begin(size, _startAddress, _maxSize)) {
        _lastError = Update.getError();
        Update.printError(error);
        error.trim(); // remove line ending
        SUPDATE_DEBUG("[httpUpdate] Update.begin failed! (%s)", error.c_str());
        return false;
    }

    if(NULL != _progressCb) {
        Update.setSendProgressCb(_progressCb);
    }

    if(md5.length()) {
        if(!Update.setMD5(md5.c_str())) {
            _lastError = HTTP_UE_SERVER_FAULTY_MD5;
            SUPDATE_DEBUG("[httpUpdate] Update.setMD5 failed! (%s)", md5.c_str());
            return false;
        }
    }

    if(Update.writeStream(in) != size) {
        _lastError = Update.getError();
        Update.printError(error);
        error.trim(); // remove line ending
        SUPDATE_DEBUG("[httpUpdate] Update.writeStream failed! (%s)", error.c_str());
        return false;
    }

    _size = Update.size();

    if(!Update.end()) {
        _size = 0;
        _lastError = Update.getError();
        Update.printError(error);
        error.trim(); // remove line ending
        SUPDATE_DEBUG("[httpUpdate] Update.end failed! (%s)", error.c_str());
        return false;
    }

    return true;
}

#endif
