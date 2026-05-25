/* ABOUTME: Data model for 3D printer status.
   ABOUTME: Fixed-size structs suitable for classic Mac memory constraints. */

#ifndef PRINTERS_H
#define PRINTERS_H

#define MAX_PRINTERS    8
#define MAX_NAME_LEN    32
#define MAX_JOB_LEN     64
#define MAX_STATE_LEN   12
#define MAX_ERROR_LEN   64
#define MAX_ID_LEN      16
#define MAX_TYPE_LEN    12

typedef struct {
    char id[MAX_ID_LEN];
    char name[MAX_NAME_LEN];
    char type[MAX_TYPE_LEN];
    char state[MAX_STATE_LEN];
    int  progress;
    char job[MAX_JOB_LEN];
    long time_remaining;
    int  nozzle_temp;
    int  nozzle_target;
    int  bed_temp;
    int  bed_target;
    char error[MAX_ERROR_LEN];
} PrinterStatus;

typedef struct {
    PrinterStatus printers[MAX_PRINTERS];
    int           count;
    unsigned long last_update_ticks;
} PrinterList;

void PrinterList_Init(PrinterList *list);
PrinterStatus *PrinterList_FindById(PrinterList *list, const char *id);

/* Format seconds as "H:MM" or "M:SS" into buf. */
void FormatTimeRemaining(long seconds, char *buf, int bufLen);

/* Returns 1 if the printer is actively doing something (printing/paused). */
int PrinterIsActive(const PrinterStatus *p);

#endif
