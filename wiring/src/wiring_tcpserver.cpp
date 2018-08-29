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

#include "intorobot_config.h"

#ifndef configNO_NETWORK

#include "wiring_tcpclient.h"
#include "wiring_tcpserver.h"
#include "wiring_network.h"

using namespace intorobot;

const static char *TAG = "wire-tcps";

static TCPClient* s_invalid_client = NULL;

static bool inline isOpen(sock_handle_t sd)
{
   return socket_handle_valid(sd);
}

class TCPServerClient : public TCPClient
{
public:
    TCPServerClient(sock_handle_t sock) : TCPClient(sock) {}

    virtual IPAddress remoteIP() override
    {
        sock_peer_t peer;
        memset(&peer, 0, sizeof(peer));
        peer.size = sizeof(peer);
        socket_peer(sock_handle(), &peer, NULL);
        return peer.address;
    }
};

TCPServer::TCPServer(uint16_t port, network_interface_t nif) : _port(port), _nif(nif), _sock(socket_handle_invalid()), _client(socket_handle_invalid())
{
    if (!s_invalid_client) {
        s_invalid_client = new TCPClient(socket_handle_invalid());
    }
}

bool TCPServer::begin()
{
    stop();
    if(!Network.from(_nif).ready()) {
        return false;
    }

    if (socket_handle_valid(_sock)) {
        return true;
    }

    sock_result_t result = socket_create_tcp_server(_port, _nif);
    if (socket_handle_valid(result)) {
        _sock = result;
        return true;
    }
    stop();
    return false;
}

void TCPServer::stop()
{
    _client.stop();
    socket_close(_sock);
    _sock = socket_handle_invalid();
}

TCPClient TCPServer::available()
{
    sock_handle_t SOCKET_INVALID = socket_handle_invalid();

    if(_sock == SOCKET_INVALID) {
        begin();
    }

    if((!Network.from(_nif).ready()) || (_sock == SOCKET_INVALID)) {
        _sock = SOCKET_INVALID;
        _client = *s_invalid_client;
        return _client;
    }

    int sock = socket_accept(_sock);

    if (!socket_handle_valid(sock)) {
        _client = *s_invalid_client;
    } else {
        TCPServerClient client = TCPServerClient(sock);
        client._remoteIP = client.remoteIP();      // fetch the peer IP ready for the copy operator
        _client = client;
    }

    return _client;
}

size_t TCPServer::write(uint8_t b)
{
    return write(&b, 1);
}

size_t TCPServer::write(const uint8_t *buffer, size_t size)
{
    return status() ? socket_send(_sock, buffer, size) : -1;
}

uint8_t TCPServer::status()
{
    return (isOpen(_sock) && Network.from(_nif).ready() && (SOCKET_STATUS_ACTIVE == socket_active_status(_sock)));
}

#endif

