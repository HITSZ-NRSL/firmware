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

/* Includes -----------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

#include "hw_config.h"
#include "esp8266_conn.h"
#include "timer_hal.h"
#include "service_debug.h"


//#define HAL_SOCKET_DEBUG

#ifdef HAL_SOCKET_DEBUG
#define HALSOCKET_DEBUG(...)  do {DEBUG(__VA_ARGS__);}while(0)
#define HALSOCKET_DEBUG_D(...)  do {DEBUG_D(__VA_ARGS__);}while(0)
#else
#define HALSOCKET_DEBUG(...)
#define HALSOCKET_DEBUG_D(...)
#endif

#ifdef putc
#undef putc
#undef getc
#endif

extern "C" {
void* ICACHE_RAM_ATTR pvPortZalloc(size_t size, const char* file, int line);

void yield(void);
void optimistic_yield(uint32_t interval_us);

void esp_yield();
void esp_schedule();
}

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
volatile uint8_t _socketConnectConfirmed=0;
volatile uint8_t _socketDisconnected=0;
volatile uint8_t _socketSended=0;
/* Private macro ------------------------------------------------------------*/
#define PROFILE         "0"   //!< this is the psd profile used
#define MAX_SIZE        1024  //!< max expected messages (used with RX)
#define USO_MAX_WRITE   1024  //!< maximum number of bytes to write to socket (used with TX)
// num sockets
#define NUMSOCKETS      ((int)(sizeof(_sockets)/sizeof(*_sockets)))
//! test if it is a socket is ok to use
#define ISSOCKET(s)     (((s) >= 0) && ((s) < NUMSOCKETS) && (_sockets[s].handle != MDM_SOCKET_ERROR))

static volatile uint32_t esp8266_conn_timeout_start;
static volatile uint32_t esp8266_conn_timeout_duration;

inline void ARM_CONN_TIMEOUT(uint32_t dur) {
    esp8266_conn_timeout_start = HAL_Timer_Get_Milli_Seconds();
    esp8266_conn_timeout_duration = dur;
    //HALSOCKET_DEBUG("esp8266 CONN WD Set %d",(dur));
}
inline bool IS_CONN_TIMEOUT() {
    return esp8266_conn_timeout_duration && ((HAL_Timer_Get_Milli_Seconds()-esp8266_conn_timeout_start)>esp8266_conn_timeout_duration);
}

inline void CLR_CONN_TIMEOUT() {
    esp8266_conn_timeout_duration = 0;
    //HALSOCKET_DEBUG("esp8266 CONN WD Cleared, was %d", esp8266_conn_timeout_duration);
}

SockCtrl Esp8266ConnClass::_sockets[5];

Esp8266ConnClass::Esp8266ConnClass(void)
{
    memset(_sockets, 0, sizeof(_sockets));
    for (int socket = 0; socket < NUMSOCKETS; socket ++)
        _sockets[socket].handle = MDM_SOCKET_ERROR;
}

int Esp8266ConnClass::socketCreate(IpProtocol ipproto, int port)
{
    int socket;

    // find an free socket
    socket = _findSocket(MDM_SOCKET_ERROR);
    if (socket != MDM_SOCKET_ERROR) {
        _sockets[socket].handle       = socket;
        _sockets[socket].ipproto      = ipproto;
        _sockets[socket].connected  = false;
        _sockets[socket].pending    = 0;
        _sockets[socket].open       = true;
        _sockets[socket].pipe = new Pipe<char>(MAX_SIZE);
        _sockets[socket].esp8266_conn_ptr = (struct espconn *)os_zalloc(sizeof(struct espconn));
        if(MDM_IPPROTO_UDP == _sockets[socket].ipproto) {
            _sockets[socket].esp8266_conn_ptr->type = ESPCONN_UDP;
            _sockets[socket].esp8266_conn_ptr->state = ESPCONN_NONE;
            _sockets[socket].esp8266_conn_ptr->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
            _sockets[socket].esp8266_conn_ptr->proto.udp->local_port = port;
            memset(_sockets[socket].esp8266_conn_ptr->proto.udp->remote_ip,0,4);
            _sockets[socket].esp8266_conn_ptr->proto.udp->remote_port = 0;
        }
        else {
            _sockets[socket].esp8266_conn_ptr->type = ESPCONN_TCP;
            _sockets[socket].esp8266_conn_ptr->state = ESPCONN_NONE;
            _sockets[socket].esp8266_conn_ptr->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
            _sockets[socket].esp8266_conn_ptr->proto.tcp->local_port = espconn_port(); //通过函数获取可用本地端口
            memset(_sockets[socket].esp8266_conn_ptr->proto.tcp->remote_ip, 0, 4);
            _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_port = 0;
        }
        HALSOCKET_DEBUG("OK! %s Socket %d was created", (ipproto?"UDP":"TCP"), socket);
    }
    else
    {
        HALSOCKET_DEBUG("Error! socket create failed!");
    }
    return socket;
}

bool Esp8266ConnClass::socketConnect(int socket, const MDM_IP& ip, int port)
{
    bool ok = false;
    if (ISSOCKET(socket) && (!_sockets[socket].connected)) {
        if(MDM_IPPROTO_UDP == _sockets[socket].ipproto) {
            _sockets[socket].esp8266_conn_ptr->proto.udp->remote_ip[0] = (ip >> 24) & 0xFF;
            _sockets[socket].esp8266_conn_ptr->proto.udp->remote_ip[1] = (ip >> 16) & 0xFF;
            _sockets[socket].esp8266_conn_ptr->proto.udp->remote_ip[2] = (ip >> 8) & 0xFF;
            _sockets[socket].esp8266_conn_ptr->proto.udp->remote_ip[3] = ip & 0xFF;
            _sockets[socket].esp8266_conn_ptr->proto.udp->remote_port = port;
            _sockets[socket].connected = true;
            espconn_create(_sockets[socket].esp8266_conn_ptr);
            ok = true;
        }
        else {
            _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_ip[0] = (ip >> 24) & 0xFF;
            _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_ip[1] = (ip >> 16) & 0xFF;
            _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_ip[2] = (ip >> 8) & 0xFF;
            _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_ip[3] = ip & 0xFF;
            _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_port = port;
            espconn_regist_connectcb(_sockets[socket].esp8266_conn_ptr, _espconn_connect_callback);
            espconn_regist_reconcb(_sockets[socket].esp8266_conn_ptr, _espconn_reconnect_callback);
            espconn_regist_disconcb(_sockets[socket].esp8266_conn_ptr, _espconn_disconnect_callback);
            ARM_CONN_TIMEOUT(3000);
            _socketConnectConfirmed =0;
            espconn_connect(_sockets[socket].esp8266_conn_ptr);
            while (!_socketConnectConfirmed) {
                optimistic_yield(100);
                if(IS_CONN_TIMEOUT()) {
                    CLR_CONN_TIMEOUT();
                    break;
                }
            }
        }

        if(_sockets[socket].connected == true) {
            espconn_regist_recvcb(_sockets[socket].esp8266_conn_ptr, _espconn_recv_callback);
            espconn_regist_sentcb(_sockets[socket].esp8266_conn_ptr, _espconn_sent_callback);
            ok = true;
            HALSOCKET_DEBUG("OK! %s Socket %d connected! remote_ip:" IPSTR ", remote_port:%d, local_port:%d", (_sockets[socket].ipproto?"UDP":"TCP"),\
                    socket, IPNUM(ip), port, _sockets[socket].esp8266_conn_ptr->proto.tcp->local_port);
        }
        else
        {
            HALSOCKET_DEBUG("Error! %s Socket %d failed! remote_ip:" IPSTR ", remote_port:%d, local_port:%d", (_sockets[socket].ipproto?"UDP":"TCP"), \
                    socket, IPNUM(ip), port, _sockets[socket].esp8266_conn_ptr->proto.tcp->local_port);
        }
    }
    else
    {
        HALSOCKET_DEBUG("Error! Socket %d connect failed! remote_ip::" IPSTR ", remote_port:%d, local_port:%d", \
                socket, IPNUM(ip), port, _sockets[socket].esp8266_conn_ptr->proto.tcp->local_port);
    }
    return ok;
}

bool Esp8266ConnClass::socketIsConnected(int socket)
{
    bool ok = false;
    ok = ISSOCKET(socket) && _sockets[socket].connected;
    //HALSOCKET_DEBUG("socket %d is Connected: %s", socket, ok?"yes":"no");
    return ok;
}

bool Esp8266ConnClass::socketClose(int socket)
{
    bool ok = false;
    if (ISSOCKET(socket) && (_sockets[socket].connected || _sockets[socket].open))
    {
        HALSOCKET_DEBUG("OK! %s socket %d close!", (_sockets[socket].ipproto?"UDP":"TCP"), socket);
        if(MDM_IPPROTO_TCP == _sockets[socket].ipproto) {
            ARM_CONN_TIMEOUT(2000);
            _socketDisconnected =0;
            espconn_disconnect(_sockets[socket].esp8266_conn_ptr);
            while (!_socketDisconnected) {
                optimistic_yield(100);
                if(IS_CONN_TIMEOUT()) {
                    CLR_CONN_TIMEOUT();
                    break;
                }
            }
        }
        if(0 == espconn_delete(_sockets[socket].esp8266_conn_ptr))
        {
            HALSOCKET_DEBUG("OK! espconn_delete");
        }
        // Assume RESP_OK in most situations, and assume closed
        // already if we couldn't close it, even though this can
        // be false. Recovery added to socketCreate();
        _sockets[socket].connected = false;
        _sockets[socket].open = false;
        ok = true;
    }
    return ok;
}

bool Esp8266ConnClass::_socketFree(int socket)
{
    bool ok = false;
    if ((socket >= 0) && (socket < NUMSOCKETS)) {
        if (_sockets[socket].handle != MDM_SOCKET_ERROR) {
            _sockets[socket].handle     = MDM_SOCKET_ERROR;
            _sockets[socket].connected  = false;
            _sockets[socket].pending    = 0;
            _sockets[socket].open       = false;
            if(MDM_IPPROTO_UDP == _sockets[socket].ipproto) {
                HALSOCKET_DEBUG("OK! udp free");
                free(_sockets[socket].esp8266_conn_ptr->proto.udp);
                _sockets[socket].esp8266_conn_ptr->proto.udp = NULL;
            }
            else {
                HALSOCKET_DEBUG("OK! tcp free");
                free(_sockets[socket].esp8266_conn_ptr->proto.tcp);
                _sockets[socket].esp8266_conn_ptr->proto.tcp = NULL;
            }
            if (_sockets[socket].esp8266_conn_ptr) {
                HALSOCKET_DEBUG("OK! esp8266 conn ptr free");
                free(_sockets[socket].esp8266_conn_ptr);
                _sockets[socket].esp8266_conn_ptr = NULL;
            }
            if (_sockets[socket].pipe) {
                delete _sockets[socket].pipe;
                _sockets[socket].pipe = NULL;
            }
        }
        ok = true;
    }
    return ok; // only false if invalid socket
}

bool Esp8266ConnClass::socketFree(int socket)
{
    // make sure it is closed
    socketClose(socket);
    if(_socketFree(socket))
    {
        HALSOCKET_DEBUG("OK! socket %d free!", socket);
        return true;
    }
    HALSOCKET_DEBUG("Error! socket %d free failed!", socket);
    return false;
}

int Esp8266ConnClass::socketSend(int socket, const char * buf, int len)
{
    HALSOCKET_DEBUG("OK! socket:%d send data len:%d!", socket,len);
    int cnt = len;
    while (cnt > 0) {
        int blk = USO_MAX_WRITE;
        if (cnt < blk)
            blk = cnt;
        bool ok = false;
        {
            if (ISSOCKET(socket)) {
                ARM_CONN_TIMEOUT(2000);
                _socketSended =0;
                espconn_send(_sockets[socket].esp8266_conn_ptr, (uint8 *)buf, blk);
                while (!_socketSended) {
                    optimistic_yield(100);
                    if(IS_CONN_TIMEOUT()) {
                        CLR_CONN_TIMEOUT();
                        break;
                    }
                }
                ok = true;
            }
        }
        if (!ok)
            return MDM_SOCKET_ERROR;
        buf += blk;
        cnt -= blk;
    }
    return (len - cnt);
}

int Esp8266ConnClass::socketSendTo(int socket, MDM_IP ip, int port, const char * buf, int len)
{
    HALSOCKET_DEBUG("OK! socket:%d send data to ip:" IPSTR ", port:%d, len:%d!", socket,IPNUM(ip),port,len);
    int cnt = len;
    while (cnt > 0) {
        int blk = USO_MAX_WRITE;
        if (cnt < blk)
            blk = cnt;
        bool ok = false;
        {
            if (ISSOCKET(socket)) {
                if(MDM_IPPROTO_UDP == _sockets[socket].ipproto) {
                    _sockets[socket].esp8266_conn_ptr->proto.udp->remote_ip[0] = (ip >> 24) & 0xFF;
                    _sockets[socket].esp8266_conn_ptr->proto.udp->remote_ip[1] = (ip >> 16) & 0xFF;
                    _sockets[socket].esp8266_conn_ptr->proto.udp->remote_ip[2] = (ip >> 8) & 0xFF;
                    _sockets[socket].esp8266_conn_ptr->proto.udp->remote_ip[3] = ip & 0xFF;
                    _sockets[socket].esp8266_conn_ptr->proto.udp->remote_port = port;
                    //HALSOCKET_DEBUG("udp remote_ip:" IPSTR "", _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_ip[0], _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_ip[1], _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_ip[2], _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_ip[3]);
                    //HALSOCKET_DEBUG("udp remote_port:%d", _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_port);
                }
                else {
                    _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_ip[0] = (ip >> 24) & 0xFF;
                    _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_ip[1] = (ip >> 16) & 0xFF;
                    _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_ip[2] = (ip >> 8) & 0xFF;
                    _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_ip[3] = ip & 0xFF;
                    _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_port = port;
                    _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_port = port;
                    //HALSOCKET_DEBUG("tcp remote_ip:" IPSTR "", _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_ip[0], _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_ip[1], _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_ip[2], _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_ip[3]);
                    //HALSOCKET_DEBUG("tcp remote_port:%d", _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_port);
                }
                ARM_CONN_TIMEOUT(2000);
                _socketSended =0;
                espconn_send(_sockets[socket].esp8266_conn_ptr, (uint8 *)buf, blk);
                while (!_socketSended) {
                    optimistic_yield(100);
                    if(IS_CONN_TIMEOUT()) {
                        CLR_CONN_TIMEOUT();
                        break;
                    }
                }
                ok = true;
            }
        }
        if (!ok)
            return MDM_SOCKET_ERROR;
        buf += blk;
        cnt -= blk;
    }
    return (len - cnt);
}

int Esp8266ConnClass::socketReadable(int socket)
{
    int pending = MDM_SOCKET_ERROR;

    optimistic_yield(100);
    if (ISSOCKET(socket)) {
        // allow to receive unsolicited commands
        pending = _sockets[socket].pending;
    }

    //因为数据已经下发到本地 所以连接断开也可以获取剩余数据  2016-01-12 chenkaiyao
    /*
    if (ISSOCKET(socket) && _sockets[socket].connected) {
        ////DEBUG_D("socketReadable(%d)\r\n", socket);
        // allow to receive unsolicited commands
        if (_sockets[socket].connected)
            pending = _sockets[socket].pending;
    }
    */
    return pending;
}

int Esp8266ConnClass::socketRecv(int socket, char* buf, int len)
{
    if (ISSOCKET(socket)) {
//        if (_sockets[socket].connected) {  //因为数据已经下发到本地 所以连接断开也可以获取剩余数据  2016-01-12 chenkaiyao
            int available = socketReadable(socket);
            if (available>0)  {
                if (len > available)    // only read up to the amount available. When 0,
                    len = available;// skip reading and check timeout.
                _sockets[socket].pipe->get(buf,len,false);
                _sockets[socket].pending -= len;
                return len;
            }
//        }
    }
    return 0;
}

int Esp8266ConnClass::socketRecvFrom(int socket, MDM_IP* ip, int* port, char* buf, int len)
{
    if (ISSOCKET(socket)) {
        //if (_sockets[socket].connected) {   //因为数据已经下发到本地 所以连接断开也可以获取剩余数据  2016-01-12 chenkaiyao
            int available = socketReadable(socket);
            if (available>0)  {
                if (len > available)    // only read up to the amount available. When 0,
                    len = available;// skip reading and check timeout.
                _sockets[socket].pipe->get(buf,len,false);
                _sockets[socket].pending -= len;
                uint8_t ipbuf[4];
                memcpy(ipbuf, _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_ip, 4);
                *ip = IPADR(ipbuf[0], ipbuf[1], ipbuf[2], ipbuf[3]);
                *port = _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_port;
                return len;
            }
        //}
    }
    return 0;
}

int Esp8266ConnClass::_findSocket(int handle) {
    for (int socket = 0; socket < NUMSOCKETS; socket ++) {
        if (_sockets[socket].handle == handle)
            return socket;
    }
    return MDM_SOCKET_ERROR;
}

void Esp8266ConnClass::_espconn_connect_callback(void *arg) {
    HALSOCKET_DEBUG("_espconn_connect_callback");

    int socket=0;
    struct espconn *pespconn = (struct espconn *)arg;

    for (socket=0; socket < NUMSOCKETS; socket++) {
        if (ISSOCKET(socket)) {
            if((pespconn->proto.tcp->remote_port == _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_port)
                    &&(!memcmp(pespconn->proto.tcp->remote_ip, (void *)_sockets[socket].esp8266_conn_ptr->proto.tcp->remote_ip, 4))) {
                break;
            }
        }
    }
    if (NUMSOCKETS != socket) {
        _sockets[socket].connected = true;
    }
    _socketConnectConfirmed = 1;
}

void Esp8266ConnClass::_espconn_reconnect_callback(void *arg, sint8 errType) {
    HALSOCKET_DEBUG("_espconn_reconnect_callback");

    int socket=0;
    struct espconn *pespconn = (struct espconn *)arg;

    for (socket=0; socket < NUMSOCKETS; socket++) {
        if (ISSOCKET(socket)) {
            if((pespconn->proto.tcp->remote_port == _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_port)
                    &&(!memcmp(pespconn->proto.tcp->remote_ip, (void *)_sockets[socket].esp8266_conn_ptr->proto.tcp->remote_ip, 4))) {
                break;
            }
        }
    }

    if (NUMSOCKETS != socket) {
        _sockets[socket].connected = false;
    }

    _socketConnectConfirmed = 1;
}

void Esp8266ConnClass::_espconn_sent_callback(void *arg) {
    HALSOCKET_DEBUG("_espconn_sent_callback");
    _socketSended = 1;
}

void Esp8266ConnClass::_espconn_recv_callback(void *arg, char *pusrdata, unsigned short len) {
    HALSOCKET_DEBUG("_espconn_recv_callback");

    int socket=0, n=0;
    struct espconn *pespconn = (struct espconn *)arg;

    for (socket=0; socket < NUMSOCKETS; socket++) {
        if (ISSOCKET(socket)) {
            if((pespconn->proto.tcp->remote_port == _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_port)
                    &&(!memcmp(pespconn->proto.tcp->remote_ip, (void *)_sockets[socket].esp8266_conn_ptr->proto.tcp->remote_ip, 4))) {
                break;
            }
        }
    }
    if (NUMSOCKETS != socket) {
        HALSOCKET_DEBUG("Socket %d: has %d bytes pending", socket, len);
        for(n=0; n < len ; n++) {
            if (_sockets[socket].pipe->writeable()) {
                _sockets[socket].pipe->putc(pusrdata[n]);
            }
            else{
                break;
            }
        }
        _sockets[socket].pending += n;
        remot_info *premot = NULL;
        if (espconn_get_connection_info(pespconn, &premot,0) == ESPCONN_OK){
            //HALSOCKET_DEBUG("espconn_get_connection_info");
            //HALSOCKET_DEBUG("remote_ip:" IPSTR "", premot->remote_ip[0], premot->remote_ip[1], premot->remote_ip[2], premot->remote_ip[3]);
            //HALSOCKET_DEBUG("remote_port:%d", premot->remote_port);
            if(MDM_IPPROTO_UDP == _sockets[socket].ipproto) {
                memcpy(_sockets[socket].esp8266_conn_ptr->proto.udp->remote_ip, premot->remote_ip, 4);
                _sockets[socket].esp8266_conn_ptr->proto.udp->remote_port = premot->remote_port;
            }
            else {
                memcpy(_sockets[socket].esp8266_conn_ptr->proto.tcp->remote_ip, premot->remote_ip, 4);
                _sockets[socket].esp8266_conn_ptr->proto.tcp->remote_port = premot->remote_port;
            }
        }
    }
}

void Esp8266ConnClass::_espconn_disconnect_callback(void *arg) {
    HALSOCKET_DEBUG("_espconn_disconnect_callback");

    int socket=0;
    struct espconn *pespconn = (struct espconn *)arg;

    for (socket=0; socket < NUMSOCKETS; socket++) {
        if (ISSOCKET(socket)) {
            if((pespconn->proto.tcp->remote_port == _sockets[socket].esp8266_conn_ptr->proto.udp->remote_port)
                    &&(!memcmp(pespconn->proto.tcp->remote_ip, (void *)_sockets[socket].esp8266_conn_ptr->proto.udp->remote_ip, 4))) {
                break;
            }
        }
    }

    if (NUMSOCKETS != socket) {
        _sockets[socket].connected = false;
    }

    _socketDisconnected = 1;
}

