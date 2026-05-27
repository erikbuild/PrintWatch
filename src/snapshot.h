/* ABOUTME: Camera snapshot fetch, cache, and display for the Mac client.
   ABOUTME: Downloads PIMG binary from the proxy and renders via CopyBits. */

#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include "network.h"

#define SNAPSHOT_MAX_SIZE 10240

/* Fetch snapshot for the given printer from the proxy.
   Returns 1 on success, 0 on failure or no data. */
int Snapshot_Fetch(NetState *net, unsigned long ip, short port,
                   const char *host, const char *printerId);

/* Draw the cached snapshot at position (x, y). Returns 0 if no snapshot. */
int Snapshot_Draw(int x, int y);

/* Returns 1 if a snapshot is cached. */
int Snapshot_HasData(void);

#endif
