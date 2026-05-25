/* ABOUTME: Reads application config from STR# 128 resource.
   ABOUTME: Converts Pascal strings to C strings for proxy IP, port, interval. */

#include "config.h"
#include <Resources.h>
#include <TextUtils.h>
#include <string.h>
#include <stdlib.h>

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
    config->pollIntervalSecs = 15;

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
}
