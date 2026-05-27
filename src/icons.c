/* ABOUTME: Printer model icon lookup and drawing.
   ABOUTME: Maps model strings to cached ICON resource handles via substring matching. */

#include "icons.h"
#include <Resources.h>
#include <Icons.h>
#include <string.h>

#define kFirstIconID 200
#define kLastIconID  210
#define kNumIcons    (kLastIconID - kFirstIconID + 1)

typedef struct {
    const char *substring;
    short       iconID;
} IconMapping;

/* Ordered most-specific-first so "Core One" matches before shorter strings. */
static const IconMapping kIconMap[] = {
    { "Core One",  204 },
    { "Trident",   207 },
    { "Ender",     209 },
    { "MINI",      203 },
    { "Bambu",     210 },
    { "MK4",       201 },
    { "MK3",       202 },
    { "XL",        205 },
    { "V2.4",      206 },
    { "K1",        208 },
    { NULL,          0 }
};

static Handle gIconCache[kNumIcons];

void Icons_Init(void) {
    int i;
    for (i = 0; i < kNumIcons; i++) {
        gIconCache[i] = GetIcon(kFirstIconID + i);
    }
}

static Handle LookupIcon(const char *model) {
    const IconMapping *m;
    int idx;

    if (model && model[0]) {
        for (m = kIconMap; m->substring != NULL; m++) {
            if (strstr(model, m->substring) != NULL) {
                idx = m->iconID - kFirstIconID;
                if (idx >= 0 && idx < kNumIcons && gIconCache[idx]) {
                    return gIconCache[idx];
                }
            }
        }
    }
    return gIconCache[0];
}

void Icons_DrawForModel(const char *model, const Rect *iconRect) {
    Handle icon = LookupIcon(model);
    if (icon) {
        PlotIcon(iconRect, icon);
    }
}
