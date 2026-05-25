/* ABOUTME: HTTP client for polling the PrintWatch proxy.
   ABOUTME: Wraps MacTCPHelper for HTTP/1.0 GET requests over TCP. */

#ifndef NETWORK_H
#define NETWORK_H

#include <Types.h>
#include "TCPRoutines.h"

#define NET_RECV_BUF_SIZE  8192
#define NET_HTTP_BUF_SIZE  4096

enum {
    kNetOK             =  0,
    kNetNoMacTCP       = -1,
    kNetConnectFailed  = -2,
    kNetSendFailed     = -3,
    kNetRecvFailed     = -4,
    kNetBadResponse    = -5,
    kNetTimeout        = -6
};

typedef struct {
    unsigned long stream;
    bool          initialized;
    bool          cancel;
    char          httpBuf[NET_HTTP_BUF_SIZE];
    int           httpLen;
} NetState;

/* Initialize MacTCP. Call once at startup. Returns kNetOK or kNetNoMacTCP. */
int Net_Init(NetState *net);

/* Make an HTTP GET to the given IP:port/path.
   On success, outBuf points into net->httpBuf (the response body),
   and outLen is set to the body length. Returns kNetOK or error code. */
int Net_HttpGet(NetState *net, unsigned long ip, short port,
                const char *host, const char *path,
                char **outBuf, int *outLen);

/* Parse a dotted-quad IP string into a network-order unsigned long.
   Returns 0 on failure. */
unsigned long Net_ParseIP(const char *ipStr);

#endif
