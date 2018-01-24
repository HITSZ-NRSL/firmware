
#ifndef SYSTEM_UPDATE_H
#define SYSTEM_UPDATE_H

#include "intorobot_config.h"
#include "md5_builder.h"
#include "wiring_ticks.h"
#include "wiring_httpclient.h"


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void set_start_dfu_flasher_serial_speed(uint32_t speed);
void set_start_ymodem_flasher_serial_speed(uint32_t speed);

void system_lineCodingBitRateHandler(uint32_t bitrate);

#ifdef __cplusplus
}
#endif

#ifndef configNO_CLOUD

#define UPDATE_ERROR_OK                 (0)
#define UPDATE_ERROR_WRITE              (1)
#define UPDATE_ERROR_ERASE              (2)
#define UPDATE_ERROR_READ               (3)
#define UPDATE_ERROR_SPACE              (4)
#define UPDATE_ERROR_SIZE               (5)
#define UPDATE_ERROR_STREAM             (6)
#define UPDATE_ERROR_MD5                (7)


#define UPDATE_SECTOR_SIZE              0x1000

typedef void (*progressCb)(uint8_t);

class UpdaterClass {
public:
    UpdaterClass();
    /*
      Call this to check the space needed for the update
      Will return false if there is not enough space
    */
    bool begin(size_t size, uint32_t startAddress, uint32_t maxSize);

    /*
      Writes a buffer to the flash and increments the address
      Returns the amount written
    */
    size_t write(uint8_t *data, size_t len);

    /*
      Writes the remaining bytes from the Stream to the flash
      Uses readBytes() and sets UPDATE_ERROR_STREAM on timeout
      Returns the bytes written
      Should be equal to the remaining bytes when called
      Usable for slow streams like Serial
    */
    size_t writeStream(Stream &data);

    /*
      If all bytes are written
      this call will write the config to eboot
      and return true
      If there is already an update running but is not finished and !evenIfRemainanig
      or there is an error
      this will clear everything and return false
      the last error is available through getError()
      evenIfRemaining is helpfull when you update without knowing the final size first
    */
    bool end(bool evenIfRemaining = false);

    /*
      Prints the last error to an output stream
    */
    void printError(Stream &out);

    /*
      sets the expected MD5 for the firmware (hexString)
    */
    bool setMD5(const char * expected_md5);

    /*
      sets the the progress display call back for the update
    */
    bool setSendProgressCb(progressCb);

    /*
      returns the MD5 String of the sucessfully ended firmware
    */
    String md5String(void){ return _md5.toString(); }

    /*
      populated the result with the md5 bytes of the sucessfully ended firmware
    */
    void md5(uint8_t * result){ return _md5.getBytes(result); }

    //Helpers
    uint8_t getError(){ return _error; }
    void clearError(){ _error = UPDATE_ERROR_OK; }
    bool hasError(){ return _error != UPDATE_ERROR_OK; }
    bool isRunning(){ return _size > 0; }
    bool isFinished(){ return _currentAddress == (_startAddress + _size); }
    size_t size(){ return _size; }
    size_t progress(){ return _currentAddress - _startAddress; }
    size_t remaining(){ return _size - (_currentAddress - _startAddress); }

    /*
      Template to write from objects that expose
      available() and read(uint8_t*, size_t) methods
      faster than the writeStream method
      writes only what is available
    */
    template<typename T>
    size_t write(T &data){
        size_t written = 0;
        if (hasError() || !isRunning())
            return 0;

        size_t available = data.available();
        while(available) {
            if(_bufferLen + available > remaining()){
                available = remaining() - _bufferLen;
            }
            if(_bufferLen + available > UPDATE_SECTOR_SIZE) {
                size_t toBuff = UPDATE_SECTOR_SIZE - _bufferLen;
                data.read(_buffer + _bufferLen, toBuff);
                _bufferLen += toBuff;
                if(!_writeBuffer())
                    return written;
                written += toBuff;
            } else {
                data.read(_buffer + _bufferLen, available);
                _bufferLen += available;
                written += available;
                if(_bufferLen == remaining()) {
                    if(!_writeBuffer()) {
                        return written;
                    }
                }
            }
            if(remaining() == 0)
                return written;
            delay(1);
            available = data.available();
        }
        return written;
    }

private:
    void _reset();
    bool _writeBuffer();

    bool _verifyHeader(uint8_t data);
    bool _verifyEnd();

    uint8_t _error;
    uint8_t *_buffer;
    size_t _bufferLen;
    size_t _size;
    uint32_t _startAddress;
    uint32_t _currentAddress;
    uint8_t _progress;

    String _target_md5;
    MD5Builder _md5;
    progressCb _progressCb;
};

extern UpdaterClass Update;

// note we use HTTP client errors too so we start at 100
#define HTTP_UE_TOO_LESS_SPACE              (-100)
#define HTTP_UE_SERVER_NOT_REPORT_SIZE      (-101)
#define HTTP_UE_SERVER_FILE_NOT_FOUND       (-102)
#define HTTP_UE_SERVER_FORBIDDEN            (-103)
#define HTTP_UE_SERVER_WRONG_HTTP_CODE      (-104)
#define HTTP_UE_SERVER_FAULTY_MD5           (-105)
#define HTTP_UE_BIN_VERIFY_HEADER_FAILED    (-106)
#define HTTP_UE_BIN_FOR_WRONG_FLASH         (-107)

enum HTTPUpdateResult {
    HTTP_UPDATE_FAILED,
    HTTP_UPDATE_NO_UPDATES,
    HTTP_UPDATE_OK
};

typedef HTTPUpdateResult t_httpUpdate_return; // backward compatibility

class HTTPUpdate
{
public:
    HTTPUpdate(void);
    ~HTTPUpdate(void);

    // This function is deprecated, use rebootOnUpdate and the next one instead
    t_httpUpdate_return update(const String& url, const String& currentVersion = "");
    t_httpUpdate_return update(const String& host, uint16_t port, const String& uri = "/", const String& currentVersion = "");

    size_t size();
    int getLastError(void);
    String getLastErrorString(void);

    bool setStoreStartAddress(uint32_t address);
    bool setStoreMaxSize(uint32_t size);
    bool setSendProgressCb(progressCb);

protected:
    t_httpUpdate_return handleUpdate(HTTPClient& http, const String& currentVersion);
    bool runUpdate(Stream& in, uint32_t size, String md5);

    uint32_t _startAddress;
    uint32_t _maxSize;
    uint32_t _size;
    progressCb _progressCb;

    int _lastError;
    bool _rebootOnUpdate = true;
};

#endif

#endif  /* SYSTEM_UPDATE_H */

