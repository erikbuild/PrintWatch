/* ABOUTME: JSON parser for the PrintWatch proxy response format.
   ABOUTME: Wraps jsmn to extract printer status from known-schema JSON. */

#ifndef JSON_H
#define JSON_H

#include "printers.h"

/* Parse the /printers response into a PrinterList. Returns 0 on success. */
int ParsePrintersResponse(const char *json, int jsonLen, PrinterList *list);

/* Parse a single printer JSON object into a PrinterStatus. Returns 0 on success. */
int ParseSinglePrinter(const char *json, int jsonLen, PrinterStatus *printer);

#endif
