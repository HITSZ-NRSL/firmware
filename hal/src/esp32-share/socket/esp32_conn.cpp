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
#include "esp32_conn.h"

extern "C" {
#include <lwip/sockets.h>
#include <lwip/netdb.h>
}

const static char *TAG = "hal-socket";

#ifdef putc
#undef putc
#undef getc
#endif

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
#define MAX_SIZE        1024  //!< max expected messages (used with RX)
#define USO_MAX_WRITE   1024  //!< maximum number of bytes to write to socket (used with TX)
// num sockets
#define NUMSOCKETS      ((int)(sizeof(_sockets)/sizeof(*_sockets)))
//! test if it is a socket is ok to use
#define ISSOCKET(s)     (((s) >= 0) && ((s) < NUMSOCKETS) && (_sockets[s].handle != MDM_SOCKET_ERROR))

SockCtrl Esp32ConnClass::_sockets[10];

Esp32ConnClass::Esp32ConnClass(void)
{
    memset(_sockets, 0, sizeof(_sockets));
    for (int socket = 0; socket < NUMSOCKETS; socket++) {
        _sockets[socket].handle = MDM_SOCKET_ERROR;
    }
}

bool Esp32ConnClass::socketIsValid(int socket)
{
    return ISSOCKET(socket);
}

int Esp32ConnClass::socketInvalid(void)
{
    return MDM_SOCKET_ERROR;
}

int Esp32ConnClass::socketCreate(IpProtocol ipproto, int port)
{
    struct sockaddr tSocketAddr;

    // find an free socket
    int socket = _findSocket(MDM_SOCKET_ERROR);
    if (socket != MDM_SOCKET_ERROR) {
        int sockfd  = lwip_socket(AF_INET, ipproto == MDM_IPPROTO_UDP ? SOCK_DGRAM : SOCK_STREAM , 0);
        if(sockfd < 0) {
            MOLMC_LOGD(TAG, "Error! socket create failed!");
            return MDM_SOCKET_ERROR;
        }
        _sockets[socket].handle = sockfd;
        _sockets[socket].ipproto = ipproto;
        _sockets[socket].pending    = 0;
        _sockets[socket].open       = true;
        _sockets[socket].pipe = new Pipe<char>(MAX_SIZE);
        if(MDM_IPPROTO_UDP == _sockets[socket].ipproto) {
            int yes = 1;
            setsockopt(_sockets[socket].handle, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
            memset(&tSocketAddr, 0, sizeof(tSocketAddr));
            tSocketAddr.sa_family = AF_INET;
            tSocketAddr.sa_data[0] = (port & 0xFF00) >> 8;
            tSocketAddr.sa_data[1] = (port & 0x00FF);
            tSocketAddr.sa_data[2] = 0;  // Todo IPv6
            tSocketAddr.sa_data[3] = 0;
            tSocketAddr.sa_data[4] = 0;
            tSocketAddr.sa_data[5] = 0;
            bind(_sockets[socket].handle, &tSocketAddr, sizeof(tSocketAddr));
            fcntl(_sockets[socket].handle, F_SETFL, O_NONBLOCK);
            MOLMC_LOGD(TAG, "OK! %s Socket %d was created", "UDP", socket);
        } else if(MDM_IPPROTO_TCP == _sockets[socket].ipproto) {
            /*  Unimplemented: connect timeout
                struct timeval tv;
                tv.tv_sec = 2;
                tv.tv_usec = 0;
                setsockopt(_sockets[socket].handle, SOL_SOCKET, SO_CONTIMEO, (char *)&tv, sizeof(struct timeval));
                */
            MOLMC_LOGD(TAG, "OK! %s Socket %d was created", "TCP", socket);
        } else { //tcp server
            memset(&tSocketAddr, 0, sizeof(tSocketAddr));
            tSocketAddr.sa_family = AF_INET;
            tSocketAddr.sa_data[0] = (port & 0xFF00) >> 8;
            tSocketAddr.sa_data[1] = (port & 0x00FF);
            tSocketAddr.sa_data[2] = 0;  // Todo IPv6
            tSocketAddr.sa_data[3] = 0;
            tSocketAddr.sa_data[4] = 0;
            tSocketAddr.sa_data[5] = 0;
            bind(_sockets[socket].handle, &tSocketAddr, sizeof(tSocketAddr));
            listen(_sockets[socket].handle , 4);
            fcntl(_sockets[socket].handle, F_SETFL, O_NONBLOCK);
            MOLMC_LOGD(TAG, "OK! %s Socket %d was created", "TCP SERVER", socket);
        }
    } else {
        MOLMC_LOGD(TAG, "Error! socket create failed!");
    }
    return socket;
}

bool Esp32ConnClass::socketConnect(int socket, const MDM_IP& ip, int port)
{
    bool ok = false;

    if (ISSOCKET(socket) && (!_sockets[socket].connected)) {
        if(MDM_IPPROTO_UDP == _sockets[socket].ipproto) {
            ok = true;
            _sockets[socket].connected = true;
        } else {
            struct sockaddr tSocketAddr;

            memset(&tSocketAddr, 0, sizeof(struct sockaddr));
            tSocketAddr.sa_family = AF_INET;
            tSocketAddr.sa_data[0] = (port>>8) & 0xFF;
            tSocketAddr.sa_data[1] = port & 0xFF;
            tSocketAddr.sa_data[2] = (ip >> 24) & 0xFF;
            tSocketAddr.sa_data[3] = (ip >> 16) & 0xFF;
            tSocketAddr.sa_data[4] = (ip >> 8) & 0xFF;
            tSocketAddr.sa_data[5] = ip & 0xFF;
            if(connect(_sockets[socket].handle, &tSocketAddr, sizeof(tSocketAddr)) >= 0) {
                ok = true;
                _sockets[socket].connected = true;
            }
        }
    }

    if(_sockets[socket].connected) {
        MOLMC_LOGD(TAG, "OK! %s Socket %d connected! remote_ip:" IPSTR ", remote_port:%d", (_sockets[socket].ipproto?"UDP":"TCP"),\
                socket, IPNUM(ip), port);
    } else {
        MOLMC_LOGD(TAG, "Error! %s Socket %d failed! remote_ip:" IPSTR ", remote_port:%d", (_sockets[socket].ipproto?"UDP":"TCP"), \
                socket, IPNUM(ip), port);
    }
    return ok;
}

bool Esp32ConnClass::socketIsConnected(int socket)
{
    bool ok = false;
    ok = ISSOCKET(socket) && _sockets[socket].connected;
    //MOLMC_LOGD(TAG, "socket %d is Connected: %s", socket, ok?"yes":"no");
    return ok;
}

int Esp32ConnClass::socketAccept(int socket)
{
    int tcpSocket = MDM_SOCKET_ERROR;
    if (ISSOCKET(socket) && (!_sockets[socket].connected)) { // 有tcp server client 连接
        tcpSocket = _findSocket(MDM_SOCKET_ERROR);
        if (tcpSocket != MDM_SOCKET_ERROR) {
            struct sockaddr tClientAddr;
            socklen_t tAddrLen = sizeof(tClientAddr);

            int sockfd = accept(_sockets[socket].handle, &tClientAddr, &tAddrLen);
            if(sockfd >= 0){
                _sockets[tcpSocket].handle     = sockfd;
                _sockets[tcpSocket].ipproto    = MDM_IPPROTO_TCP;
                _sockets[tcpSocket].connected  = true;
                _sockets[tcpSocket].pending    = 0;
                _sockets[tcpSocket].open       = true;
                _sockets[tcpSocket].pipe = new Pipe<char>(MAX_SIZE);

                int val = 1;
                if(setsockopt(_sockets[tcpSocket].handle, SOL_SOCKET, SO_KEEPALIVE, (char*)&val, sizeof(int)) == ESP_OK) {
                    val = false;//noDelay
                    if(setsockopt(_sockets[tcpSocket].handle, IPPROTO_TCP, TCP_NODELAY, (char*)&val, sizeof(int)) == ESP_OK) {
                        MOLMC_LOGD(TAG, "OK! %s tcpSocket %d was created", "TCP server client", tcpSocket);
                        return tcpSocket;
                    }
                }
            }
        }
    }
    return tcpSocket;
}

bool Esp32ConnClass::socketClose(int socket)
{
    bool ok = false;
    if (ISSOCKET(socket) && (_sockets[socket].connected || _sockets[socket].open)) {
        MOLMC_LOGD(TAG, "OK! %s socket %d close!", (_sockets[socket].ipproto?"UDP":"TCP"), socket);
        close(_sockets[socket].handle);
        _sockets[socket].connected = false;
        _sockets[socket].open = false;
        ok = true;
    }
    return ok;
}

bool Esp32ConnClass::_socketFree(int socket)
{
    bool ok = false;

    if (ISSOCKET(socket)) {
        _sockets[socket].handle     = MDM_SOCKET_ERROR;
        _sockets[socket].connected  = false;
        _sockets[socket].pending    = 0;
        _sockets[socket].open       = false;
        if (_sockets[socket].pipe) {
            delete _sockets[socket].pipe;
            _sockets[socket].pipe = NULL;
        }
        ok = true;
    }
    return ok; // only false if invalid socket
}

bool Esp32ConnClass::socketFree(int socket)
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

int Esp32ConnClass::socketSend(int socket, const char * buf, int len)
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
                int res = send(_sockets[socket].handle, (uint8_t *)buf, blk, MSG_DONTWAIT);
                if(res < 0) {
                    break;
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

int Esp32ConnClass::socketSendTo(int socket, MDM_IP ip, int port, const char * buf, int len)
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
                struct sockaddr tSocketAddr;

                memset(&tSocketAddr, 0, sizeof(struct sockaddr));
                tSocketAddr.sa_family = AF_INET;
                tSocketAddr.sa_data[0] = (port>>8) & 0xFF;
                tSocketAddr.sa_data[1] = port & 0xFF;
                tSocketAddr.sa_data[2] = (ip >> 24) & 0xFF;
                tSocketAddr.sa_data[3] = (ip >> 16) & 0xFF;
                tSocketAddr.sa_data[4] = (ip >> 8) & 0xFF;
                tSocketAddr.sa_data[5] = ip & 0xFF;
                int res = sendto(_sockets[socket].handle, buf, blk, 0, &tSocketAddr, sizeof(tSocketAddr));
                if(res < 0) {
                    break;
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

int Esp32ConnClass::socketReadable(int socket)
{
    uint8_t buffer[512] = {0};
    uint16_t readLen = 0;

    if (ISSOCKET(socket) && (_sockets[socket].connected)) {
        int count;
        int res = ioctl(_sockets[socket].handle, FIONREAD, &count);
        if(res < 0) {
            MOLMC_LOGD(TAG, "ERROR! socket : %d !", socket);
            _sockets[socket].connected  = false;
        }

        struct sockaddr si_other;
        socklen_t slen = sizeof(si_other);

        if(MDM_IPPROTO_UDP == _sockets[socket].ipproto) {
            readLen = 512; //实测 udp 有数据只能获取一次
        } else {
            readLen = 128; //实测 buffer的大小要比 tcp buffer小，否则fig 在线烧录不成功
        }

        res = recvfrom(_sockets[socket].handle, buffer, readLen, MSG_DONTWAIT, &si_other, &slen);
        if (res <= 0) {
            switch (errno) {
                case ENOTCONN:
                case EPIPE:
                case ECONNRESET:
                case ECONNREFUSED:
                case ECONNABORTED:
                    _sockets[socket].connected  = false;
                    break;
                default:
                    break;
            }
        } else {
            _sockets[socket].remote_port = si_other.sa_data[0]<<8 | si_other.sa_data[1];
            _sockets[socket].remote_ip = IPADR(si_other.sa_data[2], si_other.sa_data[3], si_other.sa_data[4], si_other.sa_data[5]);
            MOLMC_LOGD(TAG, "OK! socket:%d rev data, remote_ip:" IPSTR ", port:%d, len:%d!", socket, IPNUM(_sockets[socket].remote_ip), _sockets[socket].remote_port, res);
            int n=0;
            for(n=0; n < res ; n++) {
                if (_sockets[socket].pipe->writeable()) {
                    _sockets[socket].pipe->putc(buffer[n]);
                } else {
                    break;
                }
            }
            _sockets[socket].pending += n;
        }
    }
    return _sockets[socket].pending;
}

int Esp32ConnClass::socketRecv(int socket, char* buf, int len)
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

int Esp32ConnClass::socketRecvFrom(int socket, MDM_IP* ip, int* port, char* buf, int len)
{
    if (ISSOCKET(socket)) {
        //if (_sockets[socket].connected) {   //因为数据已经下发到本地 所以连接断开也可以获取剩余数据  2016-01-12 chenkaiyao
            int available = socketReadable(socket);
            if (available>0)  {
                if (len > available)    // only read up to the amount available. When 0,
                    len = available;// skip reading and check timeout.
                _sockets[socket].pipe->get(buf,len,false);
                _sockets[socket].pending -= len;
                *ip = _sockets[socket].remote_ip;
                *port = _sockets[socket].remote_port;
                return len;
            }
        //}
    }
    return 0;
}

int Esp32ConnClass::_findSocket(int handle) {
    for (int socket = 0; socket < NUMSOCKETS; socket++) {
        if (_sockets[socket].handle == handle)
            return socket;
    }
    return MDM_SOCKET_ERROR;
}

