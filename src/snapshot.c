/* ABOUTME: Camera snapshot fetch, cache, and display for the Mac client.
   ABOUTME: Stores a single PIMG snapshot in memory and renders with CopyBits. */

#include "snapshot.h"
#include <Quickdraw.h>
#include <string.h>
#include <stdio.h>

static char gSnapshotBuf[SNAPSHOT_MAX_SIZE];
static int gSnapshotLen = 0;

int Snapshot_Fetch(NetState *net, unsigned long ip, short port,
                   const char *host, const char *printerId) {
    char path[64];
    int bodyLen;
    int err;

    sprintf(path, "/printers/%s/snapshot", printerId);
    err = Net_HttpGetBuf(net, ip, port, host, path,
                         gSnapshotBuf, SNAPSHOT_MAX_SIZE, &bodyLen);

    if (err != kNetOK || bodyLen < 7) {
        gSnapshotLen = 0;
        return 0;
    }

    gSnapshotLen = bodyLen;
    return 1;
}

int Snapshot_Draw(int x, int y) {
    short *header;
    short w, h, rowBytes;
    BitMap srcBits;
    Rect srcRect, dstRect;
    GrafPtr port;

    if (gSnapshotLen < 7) {
        return 0;
    }

    header = (short *)gSnapshotBuf;
    w = header[0];
    h = header[1];
    rowBytes = header[2];

    if (w <= 0 || h <= 0 || rowBytes <= 0) {
        return 0;
    }
    if (6 + rowBytes * h > gSnapshotLen) {
        return 0;
    }

    srcBits.baseAddr = gSnapshotBuf + 6;
    srcBits.rowBytes = rowBytes;
    SetRect(&srcBits.bounds, 0, 0, w, h);
    SetRect(&srcRect, 0, 0, w, h);
    SetRect(&dstRect, x, y, x + w, y + h);

    GetPort(&port);
    CopyBits(&srcBits, &port->portBits, &srcRect, &dstRect, srcCopy, NULL);

    return 1;
}

int Snapshot_HasData(void) {
    return gSnapshotLen >= 7;
}
