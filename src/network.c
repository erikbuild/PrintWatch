/* ABOUTME: HTTP/1.0 client using MacTCPHelper for the PrintWatch proxy.
   ABOUTME: Synchronous GET with GiveTime yielding for cooperative multitasking. */

#include "network.h"
#include "MacTCP.h"
#include "TCPHi.h"
#include <Events.h>
#include <string.h>
#include <stdio.h>

static void GiveTime(void) {
    EventRecord event;
    WaitNextEvent(0, &event, 1, NULL);
}

int Net_Init(NetState *net) {
    OSErr err;

    memset(net, 0, sizeof(NetState));
    err = InitNetwork();
    if (err != noErr) {
        return kNetNoMacTCP;
    }
    net->initialized = true;
    return kNetOK;
}

unsigned long Net_ParseIP(const char *ipStr) {
    unsigned long a, b, c, d;
    if (sscanf(ipStr, "%lu.%lu.%lu.%lu", &a, &b, &c, &d) != 4) {
        return 0;
    }
    if (a > 255 || b > 255 || c > 255 || d > 255) {
        return 0;
    }
    return (a << 24) | (b << 16) | (c << 8) | d;
}

int Net_HttpGet(NetState *net, unsigned long ip, short port,
                const char *host, const char *path,
                char **outBuf, int *outLen) {
    OSErr err;
    char request[256];
    int reqLen;
    unsigned short recvLen;
    int totalRecv;
    char *bodyStart;

    if (!net->initialized) {
        return kNetNoMacTCP;
    }

    net->cancel = false;

    /* Create a TCP stream */
    err = CreateStream(&net->stream, NET_RECV_BUF_SIZE, GiveTime, &net->cancel);
    if (err != noErr) {
        return kNetConnectFailed;
    }

    /* Connect to the proxy */
    err = OpenConnection(net->stream, (long)ip, port, 30, GiveTime, &net->cancel);
    if (err != noErr) {
        ReleaseStream(net->stream, GiveTime, &net->cancel);
        return kNetConnectFailed;
    }

    /* Build and send the HTTP request */
    reqLen = sprintf(request,
        "GET %s HTTP/1.0\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "\r\n",
        path, host);

    err = SendData(net->stream, request, (unsigned short)reqLen, true, GiveTime, &net->cancel);
    if (err != noErr) {
        CloseNetConnection(net->stream, GiveTime, &net->cancel);
        ReleaseStream(net->stream, GiveTime, &net->cancel);
        return kNetSendFailed;
    }

    /* Receive response until connection closes or buffer full */
    totalRecv = 0;
    while (totalRecv < NET_HTTP_BUF_SIZE - 1) {
        recvLen = NET_HTTP_BUF_SIZE - 1 - totalRecv;
        err = RecvData(net->stream, net->httpBuf + totalRecv, &recvLen,
                       false, GiveTime, &net->cancel);

        if (err == noErr && recvLen > 0) {
            totalRecv += recvLen;
        } else if (err == connectionClosing || err == connectionTerminated) {
            if (recvLen > 0) {
                totalRecv += recvLen;
            }
            break;
        } else {
            break;
        }
    }
    net->httpBuf[totalRecv] = '\0';

    /* Close and release the stream */
    CloseNetConnection(net->stream, GiveTime, &net->cancel);
    ReleaseStream(net->stream, GiveTime, &net->cancel);

    if (totalRecv == 0) {
        return kNetRecvFailed;
    }

    /* Find the body after the \r\n\r\n header separator */
    bodyStart = strstr(net->httpBuf, "\r\n\r\n");
    if (bodyStart == NULL) {
        return kNetBadResponse;
    }
    bodyStart += 4;

    *outBuf = bodyStart;
    *outLen = totalRecv - (bodyStart - net->httpBuf);
    return kNetOK;
}
