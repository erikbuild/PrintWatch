/* ABOUTME: Printer model image lookup and drawing.
   ABOUTME: Maps model strings to cached PIMG resource handles via substring matching. */

#include "icons.h"
#include <Resources.h>
#include <MacMemory.h>
#include <Quickdraw.h>
#include <string.h>

typedef struct {
    const char *substring;
    short       iconID;
} IconMapping;

#include "icon_map.inc"

#define kNumIcons (kLastIconID - kFirstIconID + 1)

static Handle gImageCache[kNumIcons];

void Icons_Init(void) {
    int i;
    for (i = 0; i < kNumIcons; i++) {
        gImageCache[i] = GetResource('PIMG', kFirstIconID + i);
    }
}

static Handle LookupImage(const char *model) {
    const IconMapping *m;
    int idx;

    if (model && model[0]) {
        for (m = kIconMap; m->substring != NULL; m++) {
            if (strstr(model, m->substring) != NULL) {
                idx = m->iconID - kFirstIconID;
                if (idx >= 0 && idx < kNumIcons && gImageCache[idx]) {
                    return gImageCache[idx];
                }
            }
        }
    }
    idx = kGenericIconID - kFirstIconID;
    if (idx >= 0 && idx < kNumIcons) {
        return gImageCache[idx];
    }
    return NULL;
}

int Icons_GetSize(const char *model, int *width, int *height) {
    Handle h = LookupImage(model);
    short *header;

    *width = 0;
    *height = 0;
    if (!h || !*h) {
        return 0;
    }

    header = (short *)*h;
    *width = header[0];
    *height = header[1];
    return 1;
}

void Icons_Draw(const char *model, int x, int y) {
    Handle h = LookupImage(model);
    short *header;
    short w, ht, rowBytes;
    BitMap srcBits;
    Rect srcRect, dstRect;
    GrafPtr port;

    if (!h || !*h) {
        return;
    }

    HLock(h);
    header = (short *)*h;
    w = header[0];
    ht = header[1];
    rowBytes = header[2];

    srcBits.baseAddr = *h + 6;
    srcBits.rowBytes = rowBytes;
    SetRect(&srcBits.bounds, 0, 0, w, ht);
    SetRect(&srcRect, 0, 0, w, ht);
    SetRect(&dstRect, x, y, x + w, y + ht);

    GetPort(&port);
    CopyBits(&srcBits, &port->portBits, &srcRect, &dstRect, srcCopy, NULL);
    HUnlock(h);
}
