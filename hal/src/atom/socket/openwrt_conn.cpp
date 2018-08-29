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
#include "openwrt_conn.h"
#include "tick_hal.h"

#ifdef putc
#undef putc
#undef getc
#endif

const static char *TAG = "hal";

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
#define MAX_SIZE        256   //!< max expected messages (used with RX)
#define USO_MAX_WRITE   1024  //!< maximum number of bytes to write to socket (used with TX)
// num sockets
#define NUMSOCKETS      ((int)(sizeof(_sockets)/sizeof(*_sockets)))
//! test if it is a socket is ok to use
#define ISSOCKET(s)     (((s) >= 0) && ((s) < NUMSOCKETS) && (_sockets[s].handle != MDM_SOCKET_ERROR))

SockCtrl OpenwrtConnClass::_sockets[5];

OpenwrtConnClass::OpenwrtConnClass(BridgeClass &_b) :
    bridge(_b)
{
    memset(_sockets, 0, sizeof(_sockets));
    for (int socket = 0; socket < NUMSOCKETS; socket ++)
        _sockets[socket].handle = MDM_SOCKET_ERROR;
}

int OpenwrtConnClass::socketCreate(IpProtocol ipproto, int port)
{
    int socket;

    // find an free socket
    socket = _findSocket(MDM_SOCKET_ERROR);
    if (socket != MDM_SOCKET_ERROR) {
        MOLMC_LOGD(TAG, "Socket %d: handle %d was created", socket, socket);
        _sockets[socket].handle       = socket;
        _sockets[socket].ipproto      = ipproto;
        _sockets[socket].local_port   = port;
        _sockets[socket].remote_port  = 0;
        memset((void *)_sockets[socket].remote_ip, 0, 4);
        _sockets[socket].connected  = false;
        _sockets[socket].pending    = 0;
        _sockets[socket].open       = true;
        _sockets[socket].pipe = new Pipe<char>(MAX_SIZE);
    }
    MOLMC_LOGD(TAG, "socketCreate(%s)", (ipproto?"UDP":"TCP"));
    return socket;
}

bool OpenwrtConnClass::socketConnect(int socket, const MDM_IP& ip, int port)
{
    bool ok = false;
    if (ISSOCKET(socket) && (!_sockets[socket].connected)) {
        MOLMC_LOGD(TAG, "socketConnect(%d, ip:%s port:%d)", socket, ip, port);
        _sockets[socket].remote_port  = port;
        memcpy((void *)_sockets[socket].remote_ip, &ip, 4);
        if(_sockets[socket].ipproto) {  //udp
            uint8_t cmd[] = {'e', (uint8_t)((_sockets[socket].local_port >> 8) & 0xFF), (uint8_t)(_sockets[socket].local_port & 0xFF)};
            uint8_t res[2];
            bridge.transfer(cmd, 3, res, 2);
            if (res[1] == 1)
            {
                ok = false;
                _sockets[socket].connected = false;
            }
            else
            {
                ok = true;
                _sockets[socket].handle = res[0];
                _sockets[socket].connected = true;
            }
        }
        else { //tcp
            uint8_t tmp[] = {
                'C',
                static_cast<uint8_t>(port >> 8),
                static_cast<uint8_t>(port)
            };
            uint8_t res1[1];
            char host[32];
            memset(host, 0, sizeof(host));
            sprintf(host, "%s", "" IPSTR "", IPNUM(ip));
            int l = bridge.transfer(tmp, 3, (const uint8_t *)host, strlen(host), res1, 1);
            if (l == 0)
                return 0;
            uint8_t handle = res1[0];

            // wait for connection
            uint8_t tmp2[] = { 'c', handle };
            uint8_t res2[1];
            while (true) {
                bridge.transfer(tmp2, 2, res2, 1);
                if (res2[0] == 0)
                    break;
                delay(1);
            }

            // check for successful connection
            uint8_t cmd[] = {'L', handle};
            uint8_t res[1];
            bridge.transfer(cmd, 2, res, 1);
            if(res[0] == 1)
            {
                _sockets[socket].connected = true;
                _sockets[socket].handle = handle;
                ok = true;
            }
            else
            {
                ok = false;
                _sockets[socket].connected = false;
            }
        }
    }
    return ok;
}

bool OpenwrtConnClass::socketIsConnected(int socket)
{
    bool ok = false;
    ok = ISSOCKET(socket) && _sockets[socket].connected;
    MOLMC_LOGD(TAG, "socketIsConnected(%d) %s", socket, ok?"yes":"no");
    return ok;
}

bool OpenwrtConnClass::socketClose(int socket)
{
    bool ok = false;
    if (ISSOCKET(socket)
            && (_sockets[socket].connected || _sockets[socket].open))
    {
        if(_sockets[socket].ipproto) //udp
        {
            uint8_t cmd[] = {'j', (uint8_t)_sockets[socket].handle};
            bridge.transfer(cmd, 2);
        }
        else
        {
            uint8_t cmd[] = {'q', (uint8_t)_sockets[socket].handle};
            bridge.transfer(cmd, 2);
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

bool OpenwrtConnClass::_socketFree(int socket)
{
    bool ok = false;
    if ((socket >= 0) && (socket < NUMSOCKETS)) {
        if (_sockets[socket].handle != MDM_SOCKET_ERROR) {
            MOLMC_LOGD(TAG, "socketFree(%d)",  socket);
            _sockets[socket].handle     = MDM_SOCKET_ERROR;
            _sockets[socket].local_port    = 0;
            _sockets[socket].remote_port  = 0;
            memset((void *)_sockets[socket].remote_ip, 0, 4);
            _sockets[socket].connected  = false;
            _sockets[socket].pending    = 0;
            _sockets[socket].open       = false;
            if (_sockets[socket].pipe)
                delete _sockets[socket].pipe;
        }
        ok = true;
    }
    return ok; // only false if invalid socket
}

bool OpenwrtConnClass::socketFree(int socket)
{
    // make sure it is closed
    socketClose(socket);
    return _socketFree(socket);
}

int OpenwrtConnClass::socketSend(int socket, const char * buf, int len)
{
    MOLMC_LOGD(TAG, "socketSend(%d,,%d)", socket,len);
    int cnt = len;
    while (cnt > 0) {
        int blk = USO_MAX_WRITE;
        if (cnt < blk)
            blk = cnt;
        bool ok = false;
        {
            if (ISSOCKET(socket)) {
                uint8_t cmd[] = {'l', (uint8_t)_sockets[socket].handle};
                bridge.transfer(cmd, 2, (uint8_t *)buf, blk, NULL, 0);
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

int OpenwrtConnClass::socketSendTo(int socket, MDM_IP ip, int port, const char * buf, int len)
{
    MOLMC_LOGD(TAG, "socketSendTo(%d," IPSTR ",%d,,%d)", socket,IPNUM(ip),port,len);
    //Todo 后期实现
    return socketSend(socket, buf, len);
}

int OpenwrtConnClass::socketReadable(int socket)
{
    uint8_t buffer[255];
    int pending = MDM_SOCKET_ERROR;

    if (ISSOCKET(socket) && _sockets[socket].connected) {
        MOLMC_LOGD(TAG, "socketReadable(%d)", socket);
        // allow to receive unsolicited commands
        if( 0 == _sockets[socket].pending )
        {
            uint8_t cmd[] = {'K', (uint8_t)_sockets[socket].handle, sizeof(buffer)};
            int len = bridge.transfer(cmd, 3, buffer, sizeof(buffer));
            int n=0;
            for(n=0; n < len ; n++) {
                if (_sockets[socket].pipe->writeable()) {
                    _sockets[socket].pipe->putc(buffer[n]);
                }
                else{
                    break;
                }
            }
            _sockets[socket].pending += n;
        }
        pending = _sockets[socket].pending;
    }
    return pending;
}

int OpenwrtConnClass::socketRecv(int socket, char* buf, int len)
{
    if (ISSOCKET(socket)) {
        if (_sockets[socket].connected) {
            int available = socketReadable(socket);
            if (available>0)  {
                if (len > available)    // only read up to the amount available. When 0,
                    len = available;// skip reading and check timeout.
                _sockets[socket].pipe->get(buf,len,false);
                _sockets[socket].pending -= len;
                return len;
            }
        }
    }
    return 0;
}

int OpenwrtConnClass::socketRecvFrom(int socket, MDM_IP* ip, int* port, char* buf, int len)
{
    return socketRecv(socket, buf, len);
}

int OpenwrtConnClass::_findSocket(int handle) {
    for (int socket = 0; socket < NUMSOCKETS; socket ++) {
        if (_sockets[socket].handle == handle)
            return socket;
    }
    return MDM_SOCKET_ERROR;
}

