/******************************************************************************
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
******************************************************************************/

#include <string.h>
#include "socket_hal.h"
extern "C" {
#include <lwip/sockets.h>
#include <lwip/netdb.h>
}
//#define HAL_SOCKET_DEBUG

#ifdef HAL_SOCKET_DEBUG
#define HALSOCKET_DEBUG(...)  do {DEBUG(__VA_ARGS__);}while(0)
#define HALSOCKET_DEBUG_D(...)  do {DEBUG_D(__VA_ARGS__);}while(0)
#else
#define HALSOCKET_DEBUG(...)
#define HALSOCKET_DEBUG_D(...)
#endif

const sock_handle_t SOCKET_MAX = (sock_handle_t)5; // 5 total sockets, handle 0-4
const sock_handle_t SOCKET_INVALID = (sock_handle_t)-1;

sock_handle_t socket_create(uint8_t family, uint8_t type, uint8_t protocol, uint16_t port, network_interface_t nif)
{
    sock_handle_t handle = socket(AF_INET, protocol==IPPROTO_TCP ? SOCK_STREAM : SOCK_DGRAM, 0);
    if (socket_handle_valid(handle) && (protocol==IPPROTO_UDP)) {
        int yes = 1;
        setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        HALSOCKET_DEBUG("port = %d", port);

        struct sockaddr tSocketAddr;
        memset(&tSocketAddr, 0, sizeof(tSocketAddr));
        tSocketAddr.sa_family = AF_INET;
        tSocketAddr.sa_data[0] = (port & 0xFF00) >> 8;
        tSocketAddr.sa_data[1] = (port & 0x00FF);
        tSocketAddr.sa_data[2] = 0;  // Todo IPv6
        tSocketAddr.sa_data[3] = 0;
        tSocketAddr.sa_data[4] = 0;
        tSocketAddr.sa_data[5] = 0;
        bind(handle, &tSocketAddr, sizeof(tSocketAddr));
        fcntl(handle, F_SETFL, O_NONBLOCK);
    } else if (socket_handle_valid(handle) && (protocol==IPPROTO_TCP)) {
        /*  Unimplemented: connect timeout
        struct timeval tv;
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        setsockopt(handle, SOL_SOCKET, SO_CONTIMEO, (char *)&tv, sizeof(struct timeval));
        */
    }
    return handle;
}

int32_t socket_connect(sock_handle_t sd, const sockaddr_t *addr, long addrlen)
{
    struct sockaddr tSocketAddr;

    memset(&tSocketAddr, 0, sizeof(struct sockaddr));
    tSocketAddr.sa_family = addr->sa_family;
    memcpy(tSocketAddr.sa_data, addr->sa_data, 14);
    return connect(sd, &tSocketAddr, sizeof(tSocketAddr));
}

sock_result_t socket_reset_blocking_call()
{
    return 0;
}

sock_result_t socket_receive(sock_handle_t sd, void* buffer, socklen_t len, system_tick_t _timeout)
{
    sock_result_t result = recv(sd, buffer, len, MSG_DONTWAIT);
    return result;
}

sock_result_t socket_create_nonblocking_server(sock_handle_t sock, uint16_t port)
{
    return 0;
}

sock_result_t socket_receivefrom(sock_handle_t sock, void* buffer, socklen_t bufLen, uint32_t flags, sockaddr_t* addr, socklen_t* addrsize)
{
    struct sockaddr si_other;
    socklen_t slen = sizeof(si_other);
    sock_result_t result = recvfrom(sock, buffer, bufLen, MSG_DONTWAIT, &si_other, &slen);
    if (result > 0) {
        HALSOCKET_DEBUG("result = %d", result);
        addr->sa_family = si_other.sa_family;
        memcpy(addr->sa_data, si_other.sa_data, 14);
        HALSOCKET_DEBUG("%d %d   %d.%d.%d.%d", addr->sa_data[0], addr->sa_data[1], addr->sa_data[2],addr->sa_data[3],addr->sa_data[4],addr->sa_data[5]);
    }
    return result;
}

sock_result_t socket_bind(sock_handle_t sock, uint16_t port)
{
    return 0;
}

sock_result_t socket_accept(sock_handle_t sock)
{
    return 0;
}

uint8_t socket_active_status(sock_handle_t socket)
{
    int count;
    return (ioctl(socket, FIONREAD, &count) < 0) ? 1 : 0;
}

sock_result_t socket_close(sock_handle_t sock)
{
    return (close(sock) == 0) ? 0 : 1;
}

sock_result_t socket_send(sock_handle_t sd, const void* buffer, socklen_t len)
{
    return send(sd, buffer, len, MSG_DONTWAIT);
}

sock_result_t socket_sendto(sock_handle_t sd, const void* buffer, socklen_t len, uint32_t flags, sockaddr_t* addr, socklen_t addr_size)
{
    struct sockaddr tSocketAddr;

    memset(&tSocketAddr, 0, sizeof(struct sockaddr));
    tSocketAddr.sa_family = addr->sa_family;
    memcpy(tSocketAddr.sa_data, addr->sa_data, 14);
    return sendto(sd, buffer, len, 0, &tSocketAddr, sizeof(tSocketAddr));
}

inline bool is_valid(sock_handle_t handle) {
    return handle<SOCKET_MAX;
}

uint8_t socket_handle_valid(sock_handle_t handle) {
    return is_valid(handle);
}

sock_handle_t socket_handle_invalid()
{
    return SOCKET_INVALID;
}

sock_result_t socket_join_multicast(const HAL_IPAddress* addr, network_interface_t nif, void* reserved)
{
    return -1;
}

sock_result_t socket_leave_multicast(const HAL_IPAddress* addr, network_interface_t nif, void* reserved)
{
    return -1;
}

sock_result_t socket_peer(sock_handle_t sd, sock_peer_t* peer, void* reserved)
{
    return -1;
}

sock_result_t socket_create_tcp_server(uint16_t port, network_interface_t nif)
{
    return -1;
}
