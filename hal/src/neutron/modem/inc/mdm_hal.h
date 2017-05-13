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

#ifndef MDM_HAL_H
#define MDM_HAL_H

//#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

#include "pipe_hal.h"
#include "esp8266serialpipe_hal.h"
#include "pinmap_hal.h"
#include "system_tick_hal.h"
#include "enums_hal.h"

/* Include for debug capabilty */
//#define MODEM_DEBUG

#define MDM_ESP8266_RESET_DELAY  4000

#undef putc
#undef getc


extern "C"
{
    void init_modem_mutex(void);
}


/** basic modem parser class
*/
class MDMParser
{

public:
    //! Constructor
    MDMParser(void);
    //! get static instance
    static MDMParser* getInstance() { return inst; };

    /* Used to cancel all operations */
    void cancel(void);

    /* User to resume all operations */
    void resume(void);

    /* a hardware reset of the modem */
    void reset(void);

    /* initial the modem */
    bool init(void);

    /* get version of the modem */
    bool getNetVersion(char *version);

    /* set wifi mode  sta ap  ap+sta */
    bool setWifiMode(wifi_mode_t mode);

    /* Used to enable dhcp */
    bool setWifiDHCP(wifi_mode_t mode, char enable);

    /* Used to enable auto connection */
    bool setAutoConn(char enable);

    /* start smart config */
    bool startSmartconfig(smart_config_t type);

    /* close smart config */
    bool stopSmartconfig(void);

    /* get the smart config status*/
    deal_status_t getSmartconfigStatus(void);

    /* get local IP and mac address*/
    bool getAddress(wifi_addr_t *addr);

    /* set the sta mac an ap mac */
    bool setMacAddress(const char *staMac, const char *apMac);

    /* get the sta mac an ap mac */
    bool getMacAddress(char *staMac, char *apMac);

    /* ap scan */
    int apScan(wifi_ap_t* aps, size_t aps_count);

    /* get wifi status */
    ip_status_t getIpStatus(void);

    /* drive */
    void drive(void);

    /* get wifi info */
    bool getWifiInfo(wifi_info_t *wifiInfo);

    /* join ap */
    wifi_join_ap_t wifiJoinAp(const char *ssid, const char *password);

    wifi_join_ap_t wifiJoinAp(const char *ssid, const char *password, const char *bssid);

    /** Translates a domain name to an IP address
        \param host the domain name to translate e.g. "u-blox.com"
        \return the IP if successful, 0 otherwise
    */
    MDM_IP getHostByName(const char* host);

    /**
     * Used to ping
     */
    bool ping(const char* host);
    bool ping(const MDM_IP& ip);

    // ----------------------------------------------------------------
    // Sockets
    // ----------------------------------------------------------------

    /** Create a socket for a ip protocol (and optionaly bind)
        \param ipproto the protocol (UDP or TCP)
        \param port in case of UDP, this optional port where it is bind
        \return the socket handle if successful or SOCKET_ERROR on failure
    */
    int socketCreate(IpProtocol ipproto, int port = -1);

    /** make a socket connection
        \param socket the socket handle
        \param host the domain name to connect e.g. "u-blox.com"
        \param port the port to connect
        \return true if successfully, false otherwise
    */
    bool socketConnect(int socket, const char* host, int port);

    bool socketConnect(int socket, const MDM_IP& ip, int port);

    /** make a socket connection
        \param socket the socket handle
        \return true if connected, false otherwise
    */
    bool socketIsConnected(int socket);

    /** Write socket data
        \param socket the socket handle
        \param buf the buffer to write
        \param len the size of the buffer to write
        \return the size written or SOCKET_ERROR on failure
    */
    int socketSend(int socket, const char * buf, int len);
    int socketSendTo(int socket, MDM_IP ip, int port, const char * buf, int len);

    /** Get the number of bytes pending for reading for this socket
        \param socket the socket handle
        \return the number of bytes pending or SOCKET_ERROR on failure
    */
    int socketReadable(int socket);

    /** Read this socket
        \param socket the socket handle
        \param buf the buffer to read into
        \param len the size of the buffer to read into
        \return the number of bytes read or SOCKET_ERROR on failure
    */
    int socketRecv(int socket, char* buf, int len);

    int socketRecvFrom(int socket, MDM_IP* ip, int* port, char* buf, int len);
    /** Close a connectied socket (that was connected with #socketConnect)
        \param socket the socket handle
        \return true if successfully, false otherwise
    */
    bool socketClose(int socket);

    /** Free the socket (that was allocated before by #socketCreate)
        \param socket the socket handle
        \return true if successfully, false otherwise
    */
    bool socketFree(int socket);

    /**
     * down online ota file
     */
    deal_status_t downOtaFile(const char *host, const char *param, const char * md5);

    /**
     *  get the down online file status
     */
    deal_status_t getDownOtafileStatus(void);

    /**
     * down the net file
     */
    deal_status_t downNetFile(const char *host, const char *param);

    /**
     * Update the net
     */
    bool updateNet(void);

    /**
     * get the status of downing net file
     */
    deal_status_t getDownNetfileStatus(void);

    /**
     * get the progress of downing file
     */
    int getDownFileProgress(void);

    bool getBootloader(void);

    // ----------------------------------------------------------------
    // DEBUG/DUMP status to DEBUG output
    // ----------------------------------------------------------------

    /*! Set the debug level
        \param level -1 = OFF, 0 = ERROR, 1 = INFO(default), 2 = TRACE, 3 = ATCMD,TEST
        \return true if successful, false not possible
    */
    bool setDebug(int level);

    /** dump the ip address to DEBUG output
    */
    void dumpIp(MDM_IP ip);

    //----------------------------------------------------------------
    // Parsing
    //----------------------------------------------------------------

    /** Get a line from the physical interface. This function need
        to be implemented in a inherited class. Usually just calls
        #_getLine on the rx buffer pipe.

        \param buf the buffer to store it
        \param buf size of the buffer
        \return type and length if something was found,
                WAIT if not enough data is available
                NOT_FOUND if nothing was found
    */
    virtual int getLine(char* buf, int len) = 0;

    /* clear the pending input data
    */
    virtual void purge(void) = 0;

    /** Write data to the device
        \param buf the buffer to write
        \param buf size of the buffer to write
        \return bytes written
    */
    virtual int send(const char* buf, int len);

    /** callback function pointer typedef for SMS RESPONSE
        \param void* for optional parameter
        \param int index of the received SMS message
    */
    typedef void (*_CELLULAR_SMS_CB)(void* data, int index);

    /** Write formated date to the physical interface (printf style)
        \param fmt the format string
        \param .. variable arguments to be formated
        \return bytes written
    */
    int sendFormated(const char* format, ...);

    /** callback function for #waitFinalResp with void* as argument
        \param type the #getLine response
        \param buf the parsed line
        \param len the size of the parsed line
        \param param the optional argument passed to #waitFinalResp
        \return WAIT if processing should continue,
                any other value aborts #waitFinalResp and this retunr value retuned
    */
    typedef int (*_CALLBACKPTR)(int type, const char* buf, int len, void* param);

    /** Wait for a final respons
        \param cb the optional callback function
        \param param the optional callback function parameter
        \param timeout_ms the timeout to wait (See Estimated command
               response time of AT manual)
    */
    int waitFinalResp(_CALLBACKPTR cb = NULL,
                      void* param = NULL,
                      system_tick_t timeout_ms = 5000);

    /** template version of #waitFinalResp when using callbacks,
        This template will allow the compiler to do type cheking but
        internally symply casts the arguments and call the (void*)
        version of #waitFinalResp.
        \sa waitFinalResp
    */
    template<class T>
    inline int waitFinalResp(int (*cb)(int type, const char* buf, int len, T* param),
                    T* param,
                    system_tick_t timeout_ms = 5000)
    {
        return waitFinalResp((_CALLBACKPTR)cb, (void*)param, timeout_ms);
    }

protected:
    /** Write bytes to the physical interface. This function should be
        implemented in a inherited class.
        \param buf the buffer to write
        \param buf size of the buffer to write
        \return bytes written
    */
    virtual int _send(const void* buf, int len) = 0;

    /** Helper: Parse a line from the receiving buffered pipe
        \param pipe the receiving buffer pipe
        \param buf the parsed line
        \param len the size of the parsed line
        \return type and length if something was found,
                WAIT if not enough data is available
                NOT_FOUND if nothing was found
    */
    static int _getLine(Pipe<char>* pipe, char* buffer, int length);

    /** Helper: Parse a match from the pipe
        \param pipe the buffered pipe
        \param number of bytes to parse at maximum,
        \param sta the starting string, NULL if none
        \param end the terminating string, NULL if none
        \return size of parsed match
    */
    static int _parseMatch(Pipe<char>* pipe, int len, const char* sta, const char* end);

    /** Helper: Parse a match from the pipe
        \param pipe the buffered pipe
        \param number of bytes to parse at maximum,
        \param fmt the formating string (%d any number, %c any char of last %d len)
        \return size of parsed match
    */
    static int _parseFormated(Pipe<char>* pipe, int len, const char* fmt);

protected:
    // for rtos over riding by useing Rtos<MDMxx>
    //! override the lock in a rtos system
    virtual void lock(void)        { }
    //! override the unlock in a rtos system
    virtual void unlock(void)      { }

protected:
    // parsing callbacks for different AT commands and their parameter arguments
    static int _cbString(int type, const char* buf, int len, char* str);
    static int _cbInt(int type, const char* buf, int len, int* val);
    static int _cbGetNetVersion(int type, const char* buf, int len, char* str);
    static int _cbGetAddress(int type, const char* buf, int len, wifi_addr_t* addr);
    static int _cbGetStaMacAddress(int type, const char* buf, int len, char* str);
    static int _cbGetApMacAddress(int type, const char* buf, int len, char* str);
    static int _cbApScan(int type, const char* buf, int len, wifi_ap_t *aps);
    static int _cbGetIpStatus(int type, const char* buf, int len, ip_status_t* result);
    static int _cbGetWifiInfo(int type, const char* buf, int len, wifi_info_t *wifiInfo);
    static int _cbWifiJoinAp(int type, const char* buf, int len, wifi_join_ap_t* result);
    static int _cbGetHostByName(int type, const char* buf, int len, MDM_IP* ip);
    static int _cbDownOtaFile(int type, const char* buf, int len, deal_status_t* result);
    static int _cbDownNetFile(int type, const char* buf, int len, deal_status_t* result);
    static int _cbGetDownFileProgress(int type, const char* buf, int len, int* result);
    static int _cbGetBootloaderPacketSize(int type, const char* buf, int len, uint32_t* result);
    static int _cbGetBootloaderPacket(int type, const char* buf, int len, uint8_t* pdata);


    // management struture for sockets
    typedef struct {
        int handle;
        volatile IpProtocol ipproto;
        volatile int localip;
        volatile MDM_IP remoteip;
        volatile int remoteport;
        volatile bool connected;
        volatile int pending;
        volatile bool open;
        Pipe<char>* pipe;
    } SockCtrl;
    // LISA-C has 6 TCP and 6 UDP sockets
    // LISA-U and SARA-G have 7 sockets
    SockCtrl _sockets[7];
    int _findSocket(int handle = MDM_SOCKET_ERROR/* = CREATE*/);
    bool _socketFree(int socket);

    static MDMParser* inst;
    bool _init;

    deal_status_t _smartconfig_status;
    deal_status_t _downotafile_status;
    deal_status_t _downnetfile_status;

    static int _aplisttotalcount;
    static int _aplistindex;

    volatile bool _cancel_all_operations;
#ifdef MODEM_DEBUG
    int _debugLevel;
    system_tick_t _debugTime;
    void _debugPrint(int level, const char* color, const char* format, ...);
#endif
};

// -----------------------------------------------------------------------

/** modem class which uses USART3
    as physical interface.
*/
class MDMEsp8266Serial : public Esp8266SerialPipe, public MDMParser
{
public:
    /** Constructor
        \param rxSize the size of the serial rx buffer
        \param txSize the size of the serial tx buffer
    */
    MDMEsp8266Serial( int rxSize = 2048, int txSize = 10 );
    //! Destructor
    virtual ~MDMEsp8266Serial(void);

    /** Get a line from the physical interface.
        \param buf the buffer to store it
        \param buf size of the buffer
        \return type and length if something was found,
                WAIT if not enough data is available
                NOT_FOUND if nothing was found
    */
    virtual int getLine(char* buffer, int length);

    /* clear the pending input data */
    virtual void purge(void)
    {
        while (readable())
            getc();
    }
protected:
    /** Write bytes to the physical interface.
        \param buf the buffer to write
        \param buf size of the buffer to write
        \return bytes written
    */
    virtual int _send(const void* buf, int len);
};

/* Instance of MDMEsp8266Serial for use in HAL_USART3_Handler */
extern MDMEsp8266Serial esp8266MDM;

#endif
