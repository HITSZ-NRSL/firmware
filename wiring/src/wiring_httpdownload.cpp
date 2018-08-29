/**
 * ESP8266HTTPClient.cpp
 *
 * Created on: 02.11.2015
 *
 * Copyright (c) 2015 Markus Sattler. All rights reserved.
 * This file is part of the ESP8266HTTPClient for Arduino.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "intorobot_config.h"

#ifndef configNO_NETWORK

#include <stdlib.h>
#include "base64.h"
#include "stream_string.h"
#include "updater_hal.h"
#include "wiring_ticks.h"
#include "wiring_httpclient.h"
#include "wiring_httpdownload.h"
#include "platforms.h"

const static char *TAG = "wire-httpd";

HTTPDownload::HTTPDownload(void)
    : _lastError(0)
    , _progress_callback(NULL)
{
}

HTTPDownload::~HTTPDownload(void)
{
}

http_download_return_t HTTPDownload::update(const String& url)
{
    return update(url, "");
}

http_download_return_t HTTPDownload::update(const String& url, String md5)
{
#if PLATFORM_ID == PLATFORM_ATOM || PLATFORM_ID == PLATFORM_NEUTRON || PLATFORM_ID == PLATFORM_NUT || PLATFORM_ID == PLATFORM_W67
    String host, uri;
    uint16_t port;

    if(NULL != _progress_callback) {
        HAL_Set_Update_Handle(_progress_callback);
    }
    urlAnalyze(url, host, port, uri);
    if(HAL_Update(host.c_str(), uri.c_str(), md5.c_str(), UPDATER_MODE_UPDATE)) {
        return HTTP_DOWNLOAD_OK;
    } else {
        return HTTP_DOWNLOAD_FAILED;
    }
#else
    HTTPClient http;
    http.begin(url);
    return handleUpdate(http, md5, UPDATER_MODE_UPDATE);
#endif
}

http_download_return_t HTTPDownload::update(const String& host, uint16_t port, const String& uri, String md5)
{
#if PLATFORM_ID == PLATFORM_ATOM || PLATFORM_ID == PLATFORM_NEUTRON || PLATFORM_ID == PLATFORM_NUT || PLATFORM_ID == PLATFORM_W67
    if(HAL_Update(host.c_str(), uri.c_str(), md5.c_str(), UPDATER_MODE_UPDATE)) {
        return HTTP_DOWNLOAD_OK;
    } else {
        return HTTP_DOWNLOAD_FAILED;
    }
#else
    HTTPClient http;
    http.begin(host, port, uri);
    return handleUpdate(http, md5, UPDATER_MODE_UPDATE);
#endif
}

http_download_return_t HTTPDownload::download(const String& url)
{
    return download(url, "");
}

http_download_return_t HTTPDownload::download(const String& url, String md5)
{
#if PLATFORM_ID == PLATFORM_ATOM || PLATFORM_ID == PLATFORM_NEUTRON || PLATFORM_ID == PLATFORM_NUT || PLATFORM_ID == PLATFORM_W67
    String host, uri;
    uint16_t port;

    if(NULL != _progress_callback) {
        HAL_Set_Update_Handle(_progress_callback);
    }
    urlAnalyze(url, host, port, uri);
    if(HAL_Update(host.c_str(), uri.c_str(), md5.c_str(), UPDATER_MODE_DOWNLOAD)) {
        return HTTP_DOWNLOAD_OK;
    } else {
        return HTTP_DOWNLOAD_FAILED;
    }
#else
    HTTPClient http;
    http.begin(url);
    return handleUpdate(http, md5, UPDATER_MODE_DOWNLOAD);
#endif
}

http_download_return_t HTTPDownload::download(const String& host, uint16_t port, const String& uri, String md5)
{
#if PLATFORM_ID == PLATFORM_ATOM || PLATFORM_ID == PLATFORM_NEUTRON || PLATFORM_ID == PLATFORM_NUT || PLATFORM_ID == PLATFORM_W67
    if(HAL_Update(host.c_str(), uri.c_str(), md5.c_str(), UPDATER_MODE_DOWNLOAD)) {
        return HTTP_DOWNLOAD_OK;
    } else {
        return HTTP_DOWNLOAD_FAILED;
    }
#else
    HTTPClient http;
    http.begin(host, port, uri);
    return handleUpdate(http, md5, UPDATER_MODE_DOWNLOAD);
#endif
}

/**
 * return error code as int
 * @return int error code
 */
int HTTPDownload::getLastError(void)
{
    return _lastError;
}

/**
 * return error code as String
 * @return String error
 */
String HTTPDownload::getLastErrorString(void)
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
        case HTTP_DE_TOO_LESS_SPACE:
            return F("To less space");
        case HTTP_DE_SERVER_NOT_REPORT_SIZE:
            return F("Server not Report Size");
        case HTTP_DE_SERVER_FILE_NOT_FOUND:
            return F("File not Found (404)");
        case HTTP_DE_SERVER_FORBIDDEN:
            return F("Forbidden (403)");
        case HTTP_DE_SERVER_WRONG_HTTP_CODE:
            return F("Wrong HTTP code");
        case HTTP_DE_SERVER_FAULTY_MD5:
            return F("Faulty MD5");
        case HTTP_DE_BIN_VERIFY_HEADER_FAILED:
            return F("Verify bin header failed");
        case HTTP_DE_BIN_FOR_WRONG_FLASH:
            return F("bin for wrong flash size");
    }

    return String();
}

void HTTPDownload::onProgress(THandlerFunction_Progress fn)
{
    _progress_callback = fn;
}

bool HTTPDownload::urlAnalyze(String url, String &host, uint16_t &port, String &uri)
{
    // check for : (http: or https:
    int index = url.indexOf(':');
    if(index < 0) {
        MOLMC_LOGD(TAG, "[urlAnalyze]failed to parse protocol");
        return false;
    }

    url.remove(0, (index + 3)); // remove http:// or https://

    index = url.indexOf('/');
    String _host = url.substring(0, index);
    url.remove(0, index); // remove host part

    // get Authorization
    index = _host.indexOf('@');
    if(index >= 0) {
        _host.remove(0, index + 1); // remove auth part including @
    }

    // get port
    index = _host.indexOf(':');
    if(index >= 0) {
        host = _host.substring(0, index); // hostname
        _host.remove(0, (index + 1)); // remove hostname + :
        port = _host.toInt(); // get port
    } else {
        host = _host;
    }

    uri = url;
    return true;
}

/**
 *
 * @param http HTTPClient *
 * @param currentVersion const char *
 * @return http_download_return_t
 */
http_download_return_t HTTPDownload::handleUpdate(HTTPClient& http, String md5, updater_mode_t mode)
{
    http_download_return_t ret = HTTP_DOWNLOAD_FAILED;

    // use HTTP/1.0 for update since the update handler not support any transfer Encoding
    http.useHTTP10(true);
    http.setReuse(true); /// keep-alive
    http.setTimeout(4000);
    http.setUserAgent(F("Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/30.0.1599.101 Safari/537.36"));
    http.addHeader(F("Cache-Control"), F("no-cache"));
    http.addHeader(F("Accept"), F("*/*"));
    http.addHeader(F("Accept-Encoding"), F("gzip,deflate"));
    http.addHeader(F("Accept-Language"), F("zh-CN,eb-US;q=0.8"));

    const char * headerkeys[] = { "X-Md5" };
    size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);

    // track these headers
    http.collectHeaders(headerkeys, headerkeyssize);

    int code = http.GET();
    int len = http.getSize();

    if(code <= 0) {
        MOLMC_LOGD(TAG, "[httpDownload] HTTP error: %s", http.errorToString(code).c_str());
        _lastError = code;
        http.end();
        return HTTP_DOWNLOAD_FAILED;
    }

    MOLMC_LOGD(TAG, "[httpDownload] Header read fin.");
    MOLMC_LOGD(TAG, "[httpDownload] Server header:");
    MOLMC_LOGD(TAG, "[httpDownload] - code: %d", code);
    MOLMC_LOGD(TAG, "[httpDownload] - len: %d", len);

    if(http.hasHeader("X-Md5")) {
        MOLMC_LOGD(TAG, "[httpDownload] - MD5: %s", http.header("X-Md5").c_str());
    }

    switch(code) {
        case HTTP_CODE_OK:  ///< OK (Start Update)
            if(len > 0) {
                bool startUpdate = true;
                if((UPDATER_MODE_UPDATE == mode) && (len > (int)HAL_Update_FlashLength())) {
                    MOLMC_LOGD(TAG, "[httpDownload] FreeSketchSpace to low (%d) needed: %d", HAL_Update_FlashLength(), len);
                    startUpdate = false;
                }

                if(!startUpdate) {
                    _lastError = HTTP_DE_TOO_LESS_SPACE;
                    ret = HTTP_DOWNLOAD_FAILED;
                } else {
                    TCPClient * tcp = http.getStreamPtr();

                    delay(100);
                    MOLMC_LOGD(TAG, "[httpDownload] runUpdate flash...");

                    if(runUpdate(*tcp, len, http.header("X-Md5"), mode)) {
                        ret = HTTP_DOWNLOAD_OK;
                        MOLMC_LOGD(TAG, "[httpDownload] Update ok");
                        http.end();
                    } else {
                        ret = HTTP_DOWNLOAD_FAILED;
                        MOLMC_LOGD(TAG, "[httpDownload] Update failed");
                    }
                }
            } else {
                _lastError = HTTP_DE_SERVER_NOT_REPORT_SIZE;
                ret = HTTP_DOWNLOAD_FAILED;
                MOLMC_LOGD(TAG, "[httpDownload] Content-Length is 0 or not set by Server?!");
            }
            break;
        case HTTP_CODE_NOT_MODIFIED:
            ///< Not Modified (No updates)
            ret = HTTP_DOWNLOAD_NO_UPDATES;
            break;
        case HTTP_CODE_NOT_FOUND:
            _lastError = HTTP_DE_SERVER_FILE_NOT_FOUND;
            ret = HTTP_DOWNLOAD_FAILED;
            break;
        case HTTP_CODE_FORBIDDEN:
            _lastError = HTTP_DE_SERVER_FORBIDDEN;
            ret = HTTP_DOWNLOAD_FAILED;
            break;
        default:
            _lastError = HTTP_DE_SERVER_WRONG_HTTP_CODE;
            ret = HTTP_DOWNLOAD_FAILED;
            MOLMC_LOGD(TAG, "[httpDownload] HTTP Code is (%d)", code);
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
bool HTTPDownload::runUpdate(Stream& in, uint32_t size, String md5, updater_mode_t mode)
{
    StreamString error;

    if(!Update.begin(size, mode)) {
        _lastError = Update.getError();
        Update.printError(error);
        error.trim(); // remove line ending
        MOLMC_LOGD(TAG, "[httpDownload] Update.begin failed! (%s)", error.c_str());
        return false;
    }

    if(NULL != _progress_callback) {
        Update.onProgress(_progress_callback);
    }

    if(md5.length()) {
        if(!Update.setMD5(md5.c_str())) {
            _lastError = HTTP_DE_SERVER_FAULTY_MD5;
            MOLMC_LOGD(TAG, "[httpDownload] Update.setMD5 failed! (%s)", md5.c_str());
            return false;
        }
    }

    if(Update.writeStream(in) != size) {
        _lastError = Update.getError();
        Update.printError(error);
        error.trim(); // remove line ending
        MOLMC_LOGD(TAG, "[httpDownload] Update.writeStream failed! (%s)", error.c_str());
        return false;
    }

    if(!Update.end()) {
        _lastError = Update.getError();
        Update.printError(error);
        error.trim(); // remove line ending
        MOLMC_LOGD(TAG, "[httpDownload] Update.end failed! (%s)", error.c_str());
        return false;
    }

    return true;
}

#endif
