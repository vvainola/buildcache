/*
 * Copyright (c) 2019, Marcus Geelnard <m at bitsnbites dot eu>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define REDIS_SOCKCOMPAT_IMPLEMENTATION
#include "sockcompat.h"

#ifdef _WIN32
static int _initWinsock() {
    static int s_initialized = 0;
    if (!s_initialized) {
        static WSADATA wsadata;
        if (WSAStartup(MAKEWORD(2,2), &wsadata) != 0) {
            return 0;
        }
        s_initialized = 1;
    }
    return 1;
}

static void _updateErrno() {
    const int err = WSAGetLastError();
    switch (err) {
        case WSAEWOULDBLOCK:
            errno = EWOULDBLOCK;
            break;
        case WSAEINPROGRESS:
            errno = EINPROGRESS;
            break;
        case WSAEALREADY:
            errno = EALREADY;
            break;
        case WSAENOTSOCK:
            errno = ENOTSOCK;
            break;
        case WSAEDESTADDRREQ:
            errno = EDESTADDRREQ;
            break;
        case WSAEMSGSIZE:
            errno = EMSGSIZE;
            break;
        case WSAEPROTOTYPE:
            errno = EPROTOTYPE;
            break;
        case WSAENOPROTOOPT:
            errno = ENOPROTOOPT;
            break;
        case WSAEPROTONOSUPPORT:
            errno = EPROTONOSUPPORT;
            break;
        case WSAEOPNOTSUPP:
            errno = EOPNOTSUPP;
            break;
        case WSAEAFNOSUPPORT:
            errno = EAFNOSUPPORT;
            break;
        case WSAEADDRINUSE:
            errno = EADDRINUSE;
            break;
        case WSAEADDRNOTAVAIL:
            errno = EADDRNOTAVAIL;
            break;
        case WSAENETDOWN:
            errno = ENETDOWN;
            break;
        case WSAENETUNREACH:
            errno = ENETUNREACH;
            break;
        case WSAENETRESET:
            errno = ENETRESET;
            break;
        case WSAECONNABORTED:
            errno = ECONNABORTED;
            break;
        case WSAECONNRESET:
            errno = ECONNRESET;
            break;
        case WSAENOBUFS:
            errno = ENOBUFS;
            break;
        case WSAEISCONN:
            errno = EISCONN;
            break;
        case WSAENOTCONN:
            errno = ENOTCONN;
            break;
        case WSAETIMEDOUT:
            errno = ETIMEDOUT;
            break;
        case WSAECONNREFUSED:
            errno = ECONNREFUSED;
            break;
        case WSAELOOP:
            errno = ELOOP;
            break;
        case WSAENAMETOOLONG:
            errno = ENAMETOOLONG;
            break;
        case WSAEHOSTUNREACH:
            errno = EHOSTUNREACH;
            break;
        case WSAENOTEMPTY:
            errno = ENOTEMPTY;
            break;
        default:
            /* We just set a generic I/O error if we could not find a relevant error. */
            errno = EIO;
            break;
    }
}

int win32_getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res) {
    int ret = WSANO_RECOVERY;

    /* Note: This function is likely to be called before other functions, so run init here. */
    if (_initWinsock()) {
        ret = getaddrinfo(node, service, hints, res);
    }

    /* TODO(m): Do we need to convert the result? */
    return ret;
}

void win32_freeaddrinfo(struct addrinfo *res) {
    freeaddrinfo(res);
}

SOCKET win32_socket(int domain, int type, int protocol) {
    SOCKET s;

    /* Note: This function is likely to be called before other functions, so run init here. */
    if (!_initWinsock()) {
        return INVALID_SOCKET;
    }

    if ((s = socket(domain, type, protocol)) == INVALID_SOCKET) {
        _updateErrno();
    }
    return s;
}

int win32_getsockopt(SOCKET sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    int ret = getsockopt(sockfd, level, optname, (char*)optval, optlen);
    if (ret == SOCKET_ERROR) {
        _updateErrno();
        return -1;
    }
    return ret;
}

int win32_setsockopt(SOCKET sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    int ret = setsockopt(sockfd, level, optname, (const char*)optval, optlen);
    if (ret == SOCKET_ERROR) {
        _updateErrno();
        return -1;
    }
    return ret;
}

int win32_close(SOCKET fd) {
    int ret = closesocket(fd);
    if (ret == SOCKET_ERROR) {
        _updateErrno();
        return -1;
    }
    return ret;
}

ssize_t win32_recv(SOCKET sockfd, void *buf, size_t len, int flags) {
    int ret = recv(sockfd, (char*)buf, (int)len, flags);
    if (ret == SOCKET_ERROR) {
        _updateErrno();
        return -1;
    }
    return ret;
}

ssize_t win32_send(SOCKET sockfd, const void *buf, size_t len, int flags) {
    int ret = send(sockfd, (const char*)buf, (int)len, flags);
    if (ret == SOCKET_ERROR) {
        _updateErrno();
        return -1;
    }
    return ret;
}

int win32_poll(struct pollfd *fds, nfds_t nfds, int timeout) {
    int ret = WSAPoll(fds, nfds, timeout);
    if (ret == SOCKET_ERROR) {
        _updateErrno();
        return -1;
    }
    return ret;
}
#endif /* _WIN32 */
