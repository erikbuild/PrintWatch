/* ABOUTME: JSON parser wrapping jsmn for the proxy response schema.
   ABOUTME: Extracts printer status fields from compact JSON with zero allocation. */

#include "json.h"
#include <string.h>
#include <stdlib.h>

#define JSMN_STATIC
#include "jsmn.h"

#define MAX_TOKENS 256

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING &&
        (int)strlen(s) == tok->end - tok->start &&
        memcmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}

static void json_strcpy(const char *json, jsmntok_t *tok,
                         char *dest, int maxLen) {
    int len = tok->end - tok->start;
    if (len >= maxLen) {
        len = maxLen - 1;
    }
    memcpy(dest, json + tok->start, len);
    dest[len] = '\0';
}

static int json_int(const char *json, jsmntok_t *tok) {
    char buf[16];
    int len = tok->end - tok->start;
    if (len >= (int)sizeof(buf)) {
        len = (int)sizeof(buf) - 1;
    }
    memcpy(buf, json + tok->start, len);
    buf[len] = '\0';
    return atoi(buf);
}

static long json_long(const char *json, jsmntok_t *tok) {
    char buf[16];
    int len = tok->end - tok->start;
    if (len >= (int)sizeof(buf)) {
        len = (int)sizeof(buf) - 1;
    }
    memcpy(buf, json + tok->start, len);
    buf[len] = '\0';
    return atol(buf);
}

/* Parse a single printer object starting at tokens[pos].
   Returns the number of tokens consumed, or -1 on error. */
static int ParsePrinterObject(const char *json, jsmntok_t *tokens,
                                int pos, int numTokens,
                                PrinterStatus *printer) {
    int objSize;
    int i, j;

    if (tokens[pos].type != JSMN_OBJECT) {
        return -1;
    }

    objSize = tokens[pos].size;
    j = pos + 1;

    memset(printer, 0, sizeof(PrinterStatus));
    strncpy(printer->state, "offline", MAX_STATE_LEN - 1);

    for (i = 0; i < objSize && j + 1 < numTokens; i++) {
        if (jsoneq(json, &tokens[j], "id") == 0) {
            json_strcpy(json, &tokens[j + 1], printer->id, MAX_ID_LEN);
        } else if (jsoneq(json, &tokens[j], "name") == 0) {
            json_strcpy(json, &tokens[j + 1], printer->name, MAX_NAME_LEN);
        } else if (jsoneq(json, &tokens[j], "type") == 0) {
            json_strcpy(json, &tokens[j + 1], printer->type, MAX_TYPE_LEN);
        } else if (jsoneq(json, &tokens[j], "model") == 0) {
            json_strcpy(json, &tokens[j + 1], printer->model, MAX_MODEL_LEN);
        } else if (jsoneq(json, &tokens[j], "model_name") == 0) {
            json_strcpy(json, &tokens[j + 1], printer->model_name, MAX_MODEL_NAME_LEN);
        } else if (jsoneq(json, &tokens[j], "state") == 0) {
            json_strcpy(json, &tokens[j + 1], printer->state, MAX_STATE_LEN);
        } else if (jsoneq(json, &tokens[j], "progress") == 0) {
            printer->progress = json_int(json, &tokens[j + 1]);
        } else if (jsoneq(json, &tokens[j], "job") == 0) {
            json_strcpy(json, &tokens[j + 1], printer->job, MAX_JOB_LEN);
        } else if (jsoneq(json, &tokens[j], "time_remaining") == 0) {
            printer->time_remaining = json_long(json, &tokens[j + 1]);
        } else if (jsoneq(json, &tokens[j], "nozzle_temp") == 0) {
            printer->nozzle_temp = json_int(json, &tokens[j + 1]);
        } else if (jsoneq(json, &tokens[j], "nozzle_target") == 0) {
            printer->nozzle_target = json_int(json, &tokens[j + 1]);
        } else if (jsoneq(json, &tokens[j], "bed_temp") == 0) {
            printer->bed_temp = json_int(json, &tokens[j + 1]);
        } else if (jsoneq(json, &tokens[j], "bed_target") == 0) {
            printer->bed_target = json_int(json, &tokens[j + 1]);
        } else if (jsoneq(json, &tokens[j], "error") == 0) {
            json_strcpy(json, &tokens[j + 1], printer->error, MAX_ERROR_LEN);
        } else if (jsoneq(json, &tokens[j], "has_snapshot") == 0) {
            char buf[8];
            json_strcpy(json, &tokens[j + 1], buf, sizeof(buf));
            printer->has_snapshot = (strcmp(buf, "true") == 0);
        }
        j += 2;
    }

    return j - pos;
}

int ParsePrintersResponse(const char *json, int jsonLen, PrinterList *list) {
    jsmn_parser parser;
    jsmntok_t tokens[MAX_TOKENS];
    int numTokens;
    int i, j, arrayPos, arraySize;

    jsmn_init(&parser);
    numTokens = jsmn_parse(&parser, json, jsonLen, tokens, MAX_TOKENS);
    if (numTokens < 0) {
        return -1;
    }

    if (numTokens == 0 || tokens[0].type != JSMN_OBJECT) {
        return -1;
    }

    PrinterList_Init(list);

    /* Find the "printers" array in the top-level object */
    for (i = 1; i < numTokens; i++) {
        if (jsoneq(json, &tokens[i], "printers") == 0) {
            arrayPos = i + 1;
            if (arrayPos >= numTokens || tokens[arrayPos].type != JSMN_ARRAY) {
                return -1;
            }
            arraySize = tokens[arrayPos].size;
            j = arrayPos + 1;

            for (list->count = 0;
                 list->count < arraySize && list->count < MAX_PRINTERS;
                 list->count++) {
                int consumed = ParsePrinterObject(json, tokens, j, numTokens,
                                                   &list->printers[list->count]);
                if (consumed < 0) {
                    return -1;
                }
                j += consumed;
            }
            return 0;
        }
    }

    return -1;
}

int ParseSinglePrinter(const char *json, int jsonLen, PrinterStatus *printer) {
    jsmn_parser parser;
    jsmntok_t tokens[MAX_TOKENS];
    int numTokens;

    jsmn_init(&parser);
    numTokens = jsmn_parse(&parser, json, jsonLen, tokens, MAX_TOKENS);
    if (numTokens < 0) {
        return -1;
    }

    if (numTokens == 0 || tokens[0].type != JSMN_OBJECT) {
        return -1;
    }

    return ParsePrinterObject(json, tokens, 0, numTokens, printer) > 0 ? 0 : -1;
}
