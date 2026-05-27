/* ABOUTME: Reads and writes application config to STR# 128 resource.
   ABOUTME: Converts between Pascal and C strings for proxy IP, port, interval. */

#include "config.h"
#include <Resources.h>
#include <TextUtils.h>
#include <MacMemory.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static void PascalToC(const unsigned char *pstr, char *cstr, int maxLen) {
    int len = pstr[0];
    if (len >= maxLen) {
        len = maxLen - 1;
    }
    memcpy(cstr, pstr + 1, len);
    cstr[len] = '\0';
}

void Config_Load(AppConfig *config) {
    Str255 pstr;
    char buf[32];

    /* Defaults */
    strcpy(config->proxyIP, "10.0.2.2");
    config->proxyPort = 8080;
    config->pollIntervalSecs = 10;
    config->configured = 0;

    GetIndString(pstr, 128, 1);
    if (pstr[0] > 0) {
        PascalToC(pstr, config->proxyIP, sizeof(config->proxyIP));
    }

    GetIndString(pstr, 128, 2);
    if (pstr[0] > 0) {
        PascalToC(pstr, buf, sizeof(buf));
        config->proxyPort = atoi(buf);
    }

    GetIndString(pstr, 128, 3);
    if (pstr[0] > 0) {
        PascalToC(pstr, buf, sizeof(buf));
        config->pollIntervalSecs = atoi(buf);
    }

    GetIndString(pstr, 128, 4);
    if (pstr[0] > 0) {
        PascalToC(pstr, buf, sizeof(buf));
        config->configured = atoi(buf);
    }
}

static void CToPascal(const char *cstr, unsigned char *pstr) {
    int len = strlen(cstr);
    if (len > 255) len = 255;
    pstr[0] = len;
    memcpy(pstr + 1, cstr, len);
}

static void SetIndString(short strListID, short index, const unsigned char *pstr) {
    Handle h;
    short count, i;
    unsigned char *p;
    int oldLen, newLen, offset, totalLen;

    h = Get1Resource('STR#', strListID);
    if (h == NULL) return;

    HLock(h);
    p = (unsigned char *)*h;
    count = (p[0] << 8) | p[1];
    if (index < 1 || index > count) {
        HUnlock(h);
        return;
    }

    /* Walk to the target string */
    p += 2;
    for (i = 1; i < index; i++) {
        p += 1 + p[0];
    }

    oldLen = 1 + p[0];
    newLen = 1 + pstr[0];
    offset = p - (unsigned char *)*h;
    totalLen = GetHandleSize(h);

    if (newLen != oldLen) {
        HUnlock(h);
        SetHandleSize(h, totalLen + (newLen - oldLen));
        if (MemError() != noErr) return;
        HLock(h);
        p = (unsigned char *)*h + offset;

        if (newLen < oldLen) {
            BlockMoveData(p + oldLen, p + newLen, totalLen - offset - oldLen);
        } else {
            BlockMoveData(p + oldLen, p + newLen, totalLen - offset - oldLen);
        }
    }

    memcpy(p, pstr, newLen);
    HUnlock(h);
    ChangedResource(h);
    WriteResource(h);
}

void Config_Save(const AppConfig *config) {
    Str255 pstr;
    char buf[32];

    CToPascal(config->proxyIP, pstr);
    SetIndString(128, 1, pstr);

    sprintf(buf, "%d", config->proxyPort);
    CToPascal(buf, pstr);
    SetIndString(128, 2, pstr);

    sprintf(buf, "%d", config->pollIntervalSecs);
    CToPascal(buf, pstr);
    SetIndString(128, 3, pstr);

    CToPascal("1", pstr);
    SetIndString(128, 4, pstr);
}
