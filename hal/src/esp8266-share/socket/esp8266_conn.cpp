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
#include "tick_hal.h"
#include "core_hal_esp8266.h"

const static char *TAG = "hal-socket";

#ifdef putc
#undef putc
#undef getc
#endif

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
volatile uint8_t _socketConnectConfirmed=0;
volatile uint8_t _socketDisconnected=0;
volatile uint8_t _socketSended=0;
/* Private macro ------------------------------------------------------------*/
#define MAX_SIZE        1024  //!< max expected messages (used with RX)
#define USO_MAX_WRITE   1024  //!< maximum number of bytes to write to socket (used with TX)
// num sockets
#define NUMSOCKETS      ((int)(sizeof(_sockets)/sizeof(*_sockets)))
// num tcp server client socket list
#define NUMSERVERSOCKETLIST   ((int)(sizeof(_sockets[0].server_client_list)/sizeof(_sockets[0].server_client_list[0])))
//! test if it is a socket is ok to use
#define ISSOCKET(s)     (((s) >= 0) && ((s) < NUMSOCKETS) && (_sockets[s].handle != MDM_SOCKET_ERROR))

const char *ip_proto_name[] = { "TCP Client", "UDP Client", "TCP Server", "TCP Server Client" };

static volatile uint32_t esp8266_conn_timeout_start;
static volatile uint32_t esp8266_conn_timeout_duration;

inline void ARM_CONN_TIMEOUT(uint32_t dur) {
    esp8266_conn_timeout_start = HAL_Tick_Get_Milli_Seconds();
    esp8266_conn_timeout_duration = dur;
    MOLMC_LOGD(TAG, "esp8266 CONN WD Set %d",(dur));
}
inline bool IS_CONN_TIMEOUT() {
    return esp8266_conn_timeout_duration && ((HAL_Tick_Get_Milli_Seconds()-esp8266_conn_timeout_start)>esp8266_conn_timeout_duration);
}

inline void CLR_CONN_TIMEOUT() {
    esp8266_conn_timeout_duration = 0;
    MOLMC_LOGD(TAG, "esp8266 CONN WD Cleared, was %d", esp8266_conn_timeout_duration);
}

SockCtrl Esp8266ConnClass::_sockets[10];

Esp8266ConnClass::Esp8266ConnClass(void)
{
    memset(_sockets, 0, sizeof(_sockets));
    for (int socket = 0; socket < NUMSOCKETS; socket ++) {
        _sockets[socket].handle = MDM_SOCKET_ERROR;
    }
}

bool Esp8266ConnClass::socketIsValid(int socket)
{
    return ISSOCKET(socket);
}

int Esp8266ConnClass::socketInvalid(void)
{
    return MDM_SOCKET_ERROR;
}

int Esp8266ConnClass::socketCreate(IpProtocol ipproto, int port)
{
    int socket;
    struct espconn *pespconn = NULL;
    esp_tcp *pesp_tcp = NULL;

    // find an free socket
    if(MDM_IPPROTO_TCP_SERVER == _sockets[socket].ipproto) {
        socket = _findSocketServer(MDM_SOCKET_ERROR);
    } else {
        socket = _findSocket(MDM_SOCKET_ERROR);
    }
    if (socket != MDM_SOCKET_ERROR) {
        pespconn = (struct espconn *)os_zalloc(sizeof(struct espconn));
        pesp_tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
        if((NULL == pespconn) || (NULL == pesp_tcp)) {
            if(NULL == pespconn) {free(pespconn);}
            if(NULL == pesp_tcp) {free(pesp_tcp);}
            return MDM_SOCKET_ERROR;
        }
        memset(pespconn, 0, sizeof(struct espconn));
        memset(pesp_tcp, 0, sizeof(esp_tcp));
        _sockets[socket].handle     = socket;
        _sockets[socket].ipproto    = ipproto;
        _sockets[socket].connected  = false;
        _sockets[socket].pending    = 0;
        _sockets[socket].open       = true;
        _sockets[socket].conn_ptr = pespconn;
        _sockets[socket].conn_ptr->state = ESPCONN_NONE;
        _sockets[socket].conn_ptr->proto.tcp = pesp_tcp;
        if(MDM_IPPROTO_UDP == _sockets[socket].ipproto) {  //因为udp/tcp共用体，统一处理tcp使用
            _sockets[socket].conn_ptr->type = ESPCONN_UDP;
            _sockets[socket].conn_ptr->proto.tcp->local_port = port;
            _sockets[socket].pipe = new Pipe<char>(MAX_SIZE);
        } else if(MDM_IPPROTO_TCP == _sockets[socket].ipproto) {
            _sockets[socket].conn_ptr->type = ESPCONN_TCP;
            _sockets[socket].conn_ptr->proto.tcp->local_port = espconn_port(); //通过函数获取可用本地端口
            _sockets[socket].pipe = new Pipe<char>(MAX_SIZE);
        } else { //tcp server
            _sockets[socket].connected = true;
            _sockets[socket].conn_ptr->type = ESPCONN_TCP;
            _sockets[socket].conn_ptr->proto.tcp->local_port = port;
            espconn_regist_connectcb(_sockets[socket].conn_ptr, _espconn_tcp_server_listen_callback);
            espconn_accept(_sockets[socket].conn_ptr);
            espconn_regist_time(_sockets[socket].conn_ptr, 60, 0);
        }
        MOLMC_LOGD(TAG, "OK! %s Socket %d was created", ip_proto_name[_sockets[socket].ipproto], socket);
    } else {
        MOLMC_LOGD(TAG, "Error! socket create failed!");
    }
    return socket;
}

bool Esp8266ConnClass::socketConnect(int socket, const MDM_IP& ip, int port)
{
    bool ok = false;
    if (ISSOCKET(socket) && (!_sockets[socket].connected)) {
        _sockets[socket].conn_ptr->proto.tcp->remote_ip[0] = (ip >> 24) & 0xFF;
        _sockets[socket].conn_ptr->proto.tcp->remote_ip[1] = (ip >> 16) & 0xFF;
        _sockets[socket].conn_ptr->proto.tcp->remote_ip[2] = (ip >> 8) & 0xFF;
        _sockets[socket].conn_ptr->proto.tcp->remote_ip[3] = ip & 0xFF;
        _sockets[socket].conn_ptr->proto.tcp->remote_port = port;
        if(MDM_IPPROTO_UDP == _sockets[socket].ipproto) {
            _sockets[socket].connected = true;
            espconn_create(_sockets[socket].conn_ptr);
            ok = true;
        } else {
            espconn_regist_connectcb(_sockets[socket].conn_ptr, _espconn_connect_callback);
            espconn_regist_reconcb(_sockets[socket].conn_ptr, _espconn_reconnect_callback);
            espconn_regist_disconcb(_sockets[socket].conn_ptr, _espconn_disconnect_callback);
            ARM_CONN_TIMEOUT(3000);
            _socketConnectConfirmed =0;
            espconn_connect(_sockets[socket].conn_ptr);
            while (!_socketConnectConfirmed) {
                optimistic_yield(100);
                if(IS_CONN_TIMEOUT()) {
                    CLR_CONN_TIMEOUT();
                    break;
                }
            }
        }

        if(_sockets[socket].connected == true) {
            espconn_regist_recvcb(_sockets[socket].conn_ptr, _espconn_recv_callback);
            espconn_regist_sentcb(_sockets[socket].conn_ptr, _espconn_sent_callback);
            ok = true;
        }
    }

    if(ok) {
        MOLMC_LOGD(TAG, "OK! %s Socket %d connected! remote_ip:" IPSTR ", remote_port:%d, local_port:%d", ip_proto_name[_sockets[socket].ipproto],\
            socket, IPNUM(ip), port, _sockets[socket].conn_ptr->proto.tcp->local_port);
    } else {
        MOLMC_LOGD(TAG, "Error! %s Socket %d connect failed! remote_ip::" IPSTR ", remote_port:%d, local_port:%d", ip_proto_name[_sockets[socket].ipproto], \
                socket, IPNUM(ip), port, _sockets[socket].conn_ptr->proto.tcp->local_port);
    }
    return ok;
}

bool Esp8266ConnClass::socketIsConnected(int socket)
{
    bool ok = false;
    ok = ISSOCKET(socket) && _sockets[socket].connected;
    //MOLMC_LOGD(TAG, "socket %d is Connected: %s", socket, ok?"yes":"no");
    return ok;
}

int Esp8266ConnClass::socketAccept(int socket)
{
    int tcpSocket = MDM_SOCKET_ERROR;

    if (ISSOCKET(socket) && (_sockets[socket].ipproto == MDM_IPPROTO_TCP_SERVER)) {
        tcpSocket = _findSocket(MDM_SOCKET_ERROR);
        if (tcpSocket != MDM_SOCKET_ERROR) {
            for(int i=0; i < NUMSERVERSOCKETLIST; i++) {
                if (_sockets[socket].server_client_list[i].isValid && !_sockets[socket].server_client_list[i].isAccepted) {
                    //如果创建的时候 server client刚好关掉 此处有问题  TODO 优化
                    _sockets[socket].server_client_list[i].handle = tcpSocket;
                    _sockets[socket].server_client_list[i].isAccepted = true;

                    _sockets[tcpSocket].handle     = tcpSocket;
                    _sockets[tcpSocket].ipproto    = MDM_IPPROTO_TCP_SERVER_CLIENT;
                    _sockets[tcpSocket].connected  = true;
                    _sockets[tcpSocket].pending    = 0;
                    _sockets[tcpSocket].open       = true;
                    _sockets[tcpSocket].pipe = new Pipe<char>(MAX_SIZE);
                    _sockets[tcpSocket].conn_ptr = _sockets[socket].server_client_list[i].conn_ptr;
                    MOLMC_LOGD(TAG, "OK! %s socket %d was created! server socket %d", ip_proto_name[_sockets[tcpSocket].ipproto], tcpSocket, socket);
                    break;
                }
            }
        } else {
            MOLMC_LOGD(TAG, "Error! tcpSocket create failed!");
        }
    }
    return tcpSocket;
}

bool Esp8266ConnClass::socketClose(int socket)
{
    bool ok = false;
    if (ISSOCKET(socket) && (_sockets[socket].connected || _sockets[socket].open)) {
        MOLMC_LOGD(TAG, "OK! %s socket %d close!", ip_proto_name[_sockets[socket].ipproto], socket);
        if((MDM_IPPROTO_TCP == _sockets[socket].ipproto) && (_sockets[socket].connected)) {
            ARM_CONN_TIMEOUT(2000);
            _socketDisconnected =0;
            espconn_disconnect(_sockets[socket].conn_ptr);
            while (!_socketDisconnected) {
                optimistic_yield(100);
                if(IS_CONN_TIMEOUT()) {
                    CLR_CONN_TIMEOUT();
                    break;
                }
            }
        }

        //将tcp server client 设置为无效
        if(MDM_IPPROTO_TCP_SERVER_CLIENT == _sockets[socket].ipproto) {
            espconn_disconnect(_sockets[socket].conn_ptr);
            _sockets[socket].conn_ptr = NULL;
        }

        //设置连入tcp server客户端的连接状态为断开
        if(MDM_IPPROTO_TCP_SERVER == _sockets[socket].ipproto) {
            for(int i=0; i < NUMSERVERSOCKETLIST; i++) {
                if(_sockets[socket].server_client_list[i].isValid && _sockets[socket].server_client_list[i].isAccepted) {
                    _sockets[_sockets[socket].server_client_list[i].handle].connected = false;
                }
            }
        }

        if((MDM_IPPROTO_TCP_SERVER_CLIENT != _sockets[socket].ipproto)) {
            if(0 == espconn_delete(_sockets[socket].conn_ptr)) {
                MOLMC_LOGD(TAG, "OK! espconn_delete");
            }
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

    if (ISSOCKET(socket)) {
        _sockets[socket].handle     = MDM_SOCKET_ERROR;
        _sockets[socket].connected  = false;
        _sockets[socket].pending    = 0;
        _sockets[socket].open       = false;
        if (_sockets[socket].conn_ptr) {
            if (_sockets[socket].conn_ptr->proto.tcp) {
                MOLMC_LOGD(TAG, "OK! tcp free");
                free(_sockets[socket].conn_ptr->proto.tcp);
                _sockets[socket].conn_ptr->proto.tcp = NULL;
            }
            MOLMC_LOGD(TAG, "OK! esp8266 conn ptr free");
            free(_sockets[socket].conn_ptr);
            _sockets[socket].conn_ptr = NULL;
        }
        if (_sockets[socket].pipe) {
            MOLMC_LOGD(TAG, "OK! pipe free");
            delete _sockets[socket].pipe;
            _sockets[socket].pipe = NULL;
        }
        ok = true;
    }
    return ok; // only false if invalid socket
}

bool Esp8266ConnClass::socketFree(int socket)
{
    // make sure it is closed
    socketClose(socket);
    if(_socketFree(socket)) {
        MOLMC_LOGD(TAG, "OK! socket %d free!", socket);
        return true;
    }
    MOLMC_LOGD(TAG, "Error! socket %d free failed!", socket);
    return false;
}

int Esp8266ConnClass::socketSend(int socket, const char * buf, int len)
{
    MOLMC_LOGD(TAG, "OK! socket:%d send data len:%d!", socket,len);
    int cnt = len;
    while (cnt > 0) {
        int blk = USO_MAX_WRITE;
        if (cnt < blk)
            blk = cnt;
        bool ok = false;
        {
            if (ISSOCKET(socket)) {
                //设置连入tcp server客户端的连接状态为断开
                if(MDM_IPPROTO_TCP_SERVER == _sockets[socket].ipproto) {
                    for(int i=0; i < NUMSERVERSOCKETLIST; i++) {
                        if(_sockets[socket].server_client_list[i].isValid) {
                            ARM_CONN_TIMEOUT(1000);
                            _socketSended =0;
                            espconn_send(_sockets[socket].server_client_list[i].conn_ptr, (uint8 *)buf, blk);
                            while (!_socketSended) {
                                optimistic_yield(100);
                                if(IS_CONN_TIMEOUT()) {
                                    CLR_CONN_TIMEOUT();
                                    break;
                                }
                            }
                        }
                    }
                } else {
                    ARM_CONN_TIMEOUT(1000);
                    _socketSended =0;
                    espconn_send(_sockets[socket].conn_ptr, (uint8 *)buf, blk);
                    while (!_socketSended) {
                        optimistic_yield(100);
                        if(IS_CONN_TIMEOUT()) {
                            CLR_CONN_TIMEOUT();
                            break;
                        }
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
    MOLMC_LOGD(TAG, "OK! socket:%d send data to ip:" IPSTR ", port:%d, len:%d!", socket,IPNUM(ip),port,len);
    int cnt = len;
    while (cnt > 0) {
        int blk = USO_MAX_WRITE;
        if (cnt < blk)
            blk = cnt;
        bool ok = false;
        {
            if (ISSOCKET(socket)) {
                _sockets[socket].conn_ptr->proto.tcp->remote_ip[0] = (ip >> 24) & 0xFF;
                _sockets[socket].conn_ptr->proto.tcp->remote_ip[1] = (ip >> 16) & 0xFF;
                _sockets[socket].conn_ptr->proto.tcp->remote_ip[2] = (ip >> 8) & 0xFF;
                _sockets[socket].conn_ptr->proto.tcp->remote_ip[3] = ip & 0xFF;
                _sockets[socket].conn_ptr->proto.tcp->remote_port = port;
                MOLMC_LOGD(TAG, "%s remote_ip:" IPSTR ", remote_port:%d", ip_proto_name[_sockets[socket].ipproto], _sockets[socket].conn_ptr->proto.tcp->remote_ip[0], _sockets[socket].conn_ptr->proto.tcp->remote_ip[1], _sockets[socket].conn_ptr->proto.tcp->remote_ip[2], _sockets[socket].conn_ptr->proto.tcp->remote_ip[3], \
                        _sockets[socket].conn_ptr->proto.tcp->remote_port);
                ARM_CONN_TIMEOUT(2000);
                _socketSended =0;
                espconn_send(_sockets[socket].conn_ptr, (uint8 *)buf, blk);
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
                _sockets[socket].pipe->get(buf, len, false);
                _sockets[socket].pending -= len;
                uint8_t ipbuf[4];
                memcpy(ipbuf, _sockets[socket].conn_ptr->proto.tcp->remote_ip, 4);
                *ip = IPADR(ipbuf[0], ipbuf[1], ipbuf[2], ipbuf[3]);
                *port = _sockets[socket].conn_ptr->proto.tcp->remote_port;
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

int Esp8266ConnClass::_findSocketServer(int port, int handle) {
    //寻找相同port的tcp server
    for (int socket = 0; socket < NUMSOCKETS; socket++) {
        if ((_sockets[socket].handle != MDM_SOCKET_ERROR)
            && (_sockets[socket].ipproto == MDM_IPPROTO_TCP_SERVER)
            && (_sockets[socket].conn_ptr->proto.tcp->local_port == port))
            return MDM_SOCKET_ERROR;
    }

    for (int socket = 0; socket < NUMSOCKETS; socket++) {
        if (_sockets[socket].handle == handle)
            return socket;
    }
    return MDM_SOCKET_ERROR;
}

void Esp8266ConnClass::_espconn_connect_callback(void *arg) {
    MOLMC_LOGD(TAG, "_espconn_connect_callback");

    int socket=0;
    struct espconn *pespconn = (struct espconn *)arg;

    for (socket=0; socket < NUMSOCKETS; socket++) {
        if (ISSOCKET(socket)) {
            if((pespconn->proto.tcp->remote_port == _sockets[socket].conn_ptr->proto.tcp->remote_port)
                    &&(!memcmp(pespconn->proto.tcp->remote_ip, (void *)_sockets[socket].conn_ptr->proto.tcp->remote_ip, 4))) {
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
    MOLMC_LOGD(TAG, "_espconn_reconnect_callback");

    int socket=0;
    struct espconn *pespconn = (struct espconn *)arg;

    for (socket=0; socket < NUMSOCKETS; socket++) {
        if (ISSOCKET(socket)) {
            if((pespconn->proto.tcp->remote_port == _sockets[socket].conn_ptr->proto.tcp->remote_port)
                    &&(!memcmp(pespconn->proto.tcp->remote_ip, (void *)_sockets[socket].conn_ptr->proto.tcp->remote_ip, 4))) {
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
    MOLMC_LOGD(TAG, "_espconn_sent_callback");
    _socketSended = 1;
}

void Esp8266ConnClass::_espconn_recv_callback(void *arg, char *pusrdata, unsigned short len) {
    MOLMC_LOGD(TAG, "_espconn_recv_callback");

    int socket=0, n=0;
    struct espconn *pespconn = (struct espconn *)arg;

    for (socket=0; socket < NUMSOCKETS; socket++) {
        if (ISSOCKET(socket)) {
            if((pespconn->proto.tcp->remote_port == _sockets[socket].conn_ptr->proto.tcp->remote_port)
                    &&(!memcmp(pespconn->proto.tcp->remote_ip, (void *)_sockets[socket].conn_ptr->proto.tcp->remote_ip, 4))) {
                //MOLMC_LOGD(TAG, "socket = %d", socket);
                break;
            }
        }
    }
    if (NUMSOCKETS != socket) {
        MOLMC_LOGD(TAG, "Socket %d: has %d bytes pending", socket, len);
        for(n=0; n < len ; n++) {
            if (_sockets[socket].pipe->writeable()) {
                _sockets[socket].pipe->putc(pusrdata[n]);
            } else {
                break;
            }
        }
        _sockets[socket].pending += n;
        //UDP 获取remote_ip 和 remote_port
        if(MDM_IPPROTO_UDP == _sockets[socket].ipproto) {
            remot_info *premot = NULL;
            if (espconn_get_connection_info(pespconn, &premot, 0) == ESPCONN_OK){
                MOLMC_LOGD(TAG, "espconn_get_connection_info");
                MOLMC_LOGD(TAG, "remote_ip:" IPSTR "", premot->remote_ip[0], premot->remote_ip[1], premot->remote_ip[2], premot->remote_ip[3]);
                MOLMC_LOGD(TAG, "remote_port:%d", premot->remote_port);
                memcpy(_sockets[socket].conn_ptr->proto.tcp->remote_ip, premot->remote_ip, 4);
                _sockets[socket].conn_ptr->proto.tcp->remote_port = premot->remote_port;
            }
        }
    }
}

void Esp8266ConnClass::_espconn_disconnect_callback(void *arg) {
    MOLMC_LOGD(TAG, "_espconn_disconnect_callback");

    int socket=0;
    struct espconn *pespconn = (struct espconn *)arg;

    for (socket=0; socket < NUMSOCKETS; socket++) {
        if (ISSOCKET(socket)) {
            if((pespconn->proto.tcp->remote_port == _sockets[socket].conn_ptr->proto.tcp->remote_port)
                    &&(!memcmp(pespconn->proto.tcp->remote_ip, (void *)_sockets[socket].conn_ptr->proto.tcp->remote_ip, 4))) {
                break;
            }
        }
    }

    if (NUMSOCKETS != socket) {
        _sockets[socket].connected = false;
    }

    _socketDisconnected = 1;
}

void Esp8266ConnClass::_espconn_tcp_server_listen_callback(void *arg) {
    MOLMC_LOGD(TAG, "_espconn_tcp_server_listen_callback");

    int socket=0;
    struct espconn *pespconn = (struct espconn *)arg;

    for (socket=0; socket < NUMSOCKETS; socket++) {
        if (ISSOCKET(socket)) {
            if((_sockets[socket].ipproto == MDM_IPPROTO_TCP_SERVER)
                    &&(pespconn->proto.tcp->local_port == _sockets[socket].conn_ptr->proto.tcp->local_port)) {
                MOLMC_LOGD(TAG, "tcp server socket = %d, local_port = %d", socket, pespconn->proto.tcp->local_port);
                MOLMC_LOGD(TAG, "remote_ip:" IPSTR "", pespconn->proto.tcp->remote_ip[0], pespconn->proto.tcp->remote_ip[1], pespconn->proto.tcp->remote_ip[2], pespconn->proto.tcp->remote_ip[3]);
                MOLMC_LOGD(TAG, "remote_port:%d", pespconn->proto.tcp->remote_port);
                break;
            }
        }
    }

    if (NUMSOCKETS != socket) {
        // find an free socket
        for(int i=0; i < NUMSERVERSOCKETLIST; i++) {
            if (!_sockets[socket].server_client_list[i].isValid) {
                _sockets[socket].server_client_list[i].isValid = true;
                _sockets[socket].server_client_list[i].isAccepted = false;
                memcpy(_sockets[socket].server_client_list[i].connect_info.remote_ip, pespconn->proto.tcp->remote_ip, 4);
                _sockets[socket].server_client_list[i].connect_info.remote_port = pespconn->proto.tcp->remote_port;
                _sockets[socket].server_client_list[i].conn_ptr = pespconn;
                MOLMC_LOGD(TAG, "add server client, server index = %d", i);
                break;
            }
        }
    }

    espconn_regist_sentcb(pespconn, _espconn_tcp_server_sent_callback);
    espconn_regist_recvcb(pespconn, _espconn_tcp_server_recv_callback);
    espconn_regist_disconcb(pespconn, _espconn_tcp_server_disconnect_callback);
    espconn_regist_reconcb(pespconn, _espconn_tcp_server_disconnect_callback);
}

void Esp8266ConnClass::_espconn_tcp_server_sent_callback(void *arg) {
    _socketSended = 1;
    MOLMC_LOGD(TAG, "_espconn_tcp_server_sent_callback");
}

void Esp8266ConnClass::_espconn_tcp_server_recv_callback(void *arg, char *pusrdata, unsigned short len) {
    MOLMC_LOGD(TAG, "_espconn_tcp_server_recv_callback");
    int socket=0;
    struct espconn *pespconn = (struct espconn *)arg;

    for (socket=0; socket < NUMSOCKETS; socket++) {
        if (ISSOCKET(socket)) {
            if((_sockets[socket].ipproto == MDM_IPPROTO_TCP_SERVER)
                    &&(pespconn->proto.tcp->local_port == _sockets[socket].conn_ptr->proto.tcp->local_port)) {
                MOLMC_LOGD(TAG, "tcp server socket = %d, local_port = %d", socket, pespconn->proto.tcp->local_port);
                break;
            }
        }
    }

    if (NUMSOCKETS != socket) {
        for(int i=0; i < NUMSERVERSOCKETLIST; i++) {
            //tcp server client 清除。socket 设置为未连接。
            if((pespconn->proto.tcp->remote_port == _sockets[socket].server_client_list[i].connect_info.remote_port)
                    &&(!memcmp(pespconn->proto.tcp->remote_ip, (void *)_sockets[socket].server_client_list[i].connect_info.remote_ip, 4))
                    &&(_sockets[socket].server_client_list[i].isAccepted)) {
                MOLMC_LOGD(TAG, "remote_ip:" IPSTR "", pespconn->proto.tcp->remote_ip[0], pespconn->proto.tcp->remote_ip[1], pespconn->proto.tcp->remote_ip[2], pespconn->proto.tcp->remote_ip[3]);
                MOLMC_LOGD(TAG, "remote_port:%d", pespconn->proto.tcp->remote_port);
                int handle = _sockets[socket].server_client_list[i].handle;
                MOLMC_LOGD(TAG, "Socket %d: has %d bytes pending", handle, len);
                int n = 0;
                for(n=0; n < len ; n++) {
                    if (_sockets[handle].pipe->writeable()) {
                        _sockets[handle].pipe->putc(pusrdata[n]);
                    } else {
                        break;
                    }
                }
                _sockets[handle].pending += n;
                break;
            }
        }
    }
}

void Esp8266ConnClass::_espconn_tcp_server_disconnect_callback(void *arg) {
    MOLMC_LOGD(TAG, "_espconn_tcp_server_disconnect_callback");
    int socket=0;
    struct espconn *pespconn = (struct espconn *)arg;

    for (socket=0; socket < NUMSOCKETS; socket++) {
        if (ISSOCKET(socket)) {
            if((_sockets[socket].ipproto == MDM_IPPROTO_TCP_SERVER)
                    &&(pespconn->proto.tcp->local_port == _sockets[socket].conn_ptr->proto.tcp->local_port)) {
                MOLMC_LOGD(TAG, "tcp server socket = %d, local_port = %d", socket, pespconn->proto.tcp->local_port);
                break;
            }
        }
    }

    if (NUMSOCKETS != socket) {
        for(int i=0; i < NUMSERVERSOCKETLIST; i++) {
            //tcp server client 清除。socket 设置为未连接。
            if((pespconn->proto.tcp->remote_port == _sockets[socket].server_client_list[i].connect_info.remote_port)
                    &&(!memcmp(pespconn->proto.tcp->remote_ip, (void *)_sockets[socket].server_client_list[i].connect_info.remote_ip, 4))) {
                if(_sockets[socket].server_client_list[i].isAccepted) {
                    _sockets[_sockets[socket].server_client_list[i].handle].connected = false;
                }
                _sockets[socket].server_client_list[i].isValid = false;
                MOLMC_LOGD(TAG, "remote_ip:" IPSTR "", pespconn->proto.tcp->remote_ip[0], pespconn->proto.tcp->remote_ip[1], pespconn->proto.tcp->remote_ip[2], pespconn->proto.tcp->remote_ip[3]);
                MOLMC_LOGD(TAG, "remote_port:%d, server index = %d", pespconn->proto.tcp->remote_port, i);
                break;
            }
        }
    }
}

