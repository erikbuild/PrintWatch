/* ABOUTME: Printer data model implementation.
   ABOUTME: Fixed-size storage and formatting helpers for printer status. */

#include "printers.h"
#include <string.h>
#include <stdio.h>

void PrinterList_Init(PrinterList *list) {
    memset(list, 0, sizeof(PrinterList));
}

PrinterStatus *PrinterList_FindById(PrinterList *list, const char *id) {
    int i;
    for (i = 0; i < list->count; i++) {
        if (strcmp(list->printers[i].id, id) == 0) {
            return &list->printers[i];
        }
    }
    return NULL;
}

void FormatTimeRemaining(long seconds, char *buf, int bufLen) {
    long hours, mins, secs;

    if (seconds <= 0) {
        if (bufLen > 0) {
            buf[0] = '-';
            buf[1] = '\0';
        }
        return;
    }

    hours = seconds / 3600;
    mins  = (seconds % 3600) / 60;
    secs  = seconds % 60;

    if (hours > 0) {
        snprintf(buf, bufLen, "%ld:%02ld", hours, mins);
    } else {
        snprintf(buf, bufLen, "%ld:%02ld", mins, secs);
    }
}

int PrinterIsActive(const PrinterStatus *p) {
    return (strcmp(p->state, "printing") == 0 ||
            strcmp(p->state, "paused") == 0);
}
