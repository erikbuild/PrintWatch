/* ABOUTME: Host-side unit tests for the JSON parser.
   ABOUTME: Tests parsing of proxy responses into PrinterStatus structs. */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "json.h"
#include "printers.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { \
    printf("  %-50s", name); \
    tests_run++; \
} while(0)

#define PASS() do { \
    tests_passed++; \
    printf("OK\n"); \
} while(0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("FAIL (line %d: %s)\n", __LINE__, #cond); \
        return; \
    } \
} while(0)

static const char *kTwoPrinters =
    "{\"printers\":["
    "{\"id\":\"mk4\",\"name\":\"Prusa MK4\",\"type\":\"prusalink\","
    "\"state\":\"printing\",\"progress\":78,\"job\":\"benchy.gcode\","
    "\"time_remaining\":4320,\"nozzle_temp\":215,\"nozzle_target\":215,"
    "\"bed_temp\":60,\"bed_target\":60,\"error\":\"\"},"
    "{\"id\":\"voron\",\"name\":\"Voron 2.4\",\"type\":\"moonraker\","
    "\"state\":\"idle\",\"progress\":0,\"job\":\"\","
    "\"time_remaining\":0,\"nozzle_temp\":22,\"nozzle_target\":0,"
    "\"bed_temp\":21,\"bed_target\":0,\"error\":\"\"}"
    "]}";

static const char *kEmptyPrinters = "{\"printers\":[]}";

static const char *kSinglePrinter =
    "{\"id\":\"ender\",\"name\":\"Ender 3\",\"type\":\"moonraker\","
    "\"state\":\"error\",\"progress\":45,\"job\":\"bracket.gcode\","
    "\"time_remaining\":1800,\"nozzle_temp\":200,\"nozzle_target\":210,"
    "\"bed_temp\":55,\"bed_target\":60,\"error\":\"Thermal runaway\"}";

static void test_parse_two_printers(void) {
    PrinterList list;
    int result;

    TEST("Parse two printers");
    result = ParsePrintersResponse(kTwoPrinters, strlen(kTwoPrinters), &list);
    ASSERT(result == 0);
    ASSERT(list.count == 2);
    PASS();

    TEST("First printer fields correct");
    ASSERT(strcmp(list.printers[0].id, "mk4") == 0);
    ASSERT(strcmp(list.printers[0].name, "Prusa MK4") == 0);
    ASSERT(strcmp(list.printers[0].type, "prusalink") == 0);
    ASSERT(strcmp(list.printers[0].state, "printing") == 0);
    ASSERT(list.printers[0].progress == 78);
    ASSERT(strcmp(list.printers[0].job, "benchy.gcode") == 0);
    ASSERT(list.printers[0].time_remaining == 4320);
    ASSERT(list.printers[0].nozzle_temp == 215);
    ASSERT(list.printers[0].nozzle_target == 215);
    ASSERT(list.printers[0].bed_temp == 60);
    ASSERT(list.printers[0].bed_target == 60);
    ASSERT(list.printers[0].error[0] == '\0');
    PASS();

    TEST("Second printer is idle");
    ASSERT(strcmp(list.printers[1].id, "voron") == 0);
    ASSERT(strcmp(list.printers[1].state, "idle") == 0);
    ASSERT(list.printers[1].progress == 0);
    ASSERT(list.printers[1].nozzle_temp == 22);
    PASS();
}

static void test_parse_empty(void) {
    PrinterList list;
    int result;

    TEST("Parse empty printers array");
    result = ParsePrintersResponse(kEmptyPrinters, strlen(kEmptyPrinters), &list);
    ASSERT(result == 0);
    ASSERT(list.count == 0);
    PASS();
}

static void test_parse_single_printer(void) {
    PrinterStatus printer;
    int result;

    TEST("ParseSinglePrinter extracts all fields");
    result = ParseSinglePrinter(kSinglePrinter, strlen(kSinglePrinter), &printer);
    ASSERT(result == 0);
    ASSERT(strcmp(printer.id, "ender") == 0);
    ASSERT(strcmp(printer.name, "Ender 3") == 0);
    ASSERT(strcmp(printer.state, "error") == 0);
    ASSERT(printer.progress == 45);
    ASSERT(strcmp(printer.job, "bracket.gcode") == 0);
    ASSERT(printer.time_remaining == 1800);
    ASSERT(printer.nozzle_temp == 200);
    ASSERT(printer.nozzle_target == 210);
    ASSERT(printer.bed_temp == 55);
    ASSERT(printer.bed_target == 60);
    ASSERT(strcmp(printer.error, "Thermal runaway") == 0);
    PASS();
}

static void test_parse_malformed(void) {
    PrinterList list;
    int result;

    TEST("Malformed JSON returns -1");
    result = ParsePrintersResponse("{bad json", 9, &list);
    ASSERT(result == -1);
    PASS();

    TEST("Empty string returns -1");
    result = ParsePrintersResponse("", 0, &list);
    ASSERT(result == -1);
    PASS();

    TEST("Array without printers key returns -1");
    result = ParsePrintersResponse("{\"other\":[]}", 12, &list);
    ASSERT(result == -1);
    PASS();

    TEST("Non-object top level returns -1");
    result = ParsePrintersResponse("[1,2,3]", 7, &list);
    ASSERT(result == -1);
    PASS();
}

static void test_parse_large_response(void) {
    /* Simulate 6 printers (medium farm scenario) */
    static char json[4096];
    PrinterList list;
    int result, i;
    int pos = 0;

    TEST("Parse 6 printers (medium farm)");
    pos += snprintf(json + pos, sizeof(json) - pos, "{\"printers\":[");
    for (i = 0; i < 6; i++) {
        if (i > 0) {
            json[pos++] = ',';
        }
        pos += snprintf(json + pos, sizeof(json) - pos,
            "{\"id\":\"p%d\",\"name\":\"Printer %d\",\"type\":\"moonraker\","
            "\"state\":\"printing\",\"progress\":%d,\"job\":\"part%d.gcode\","
            "\"time_remaining\":%d,\"nozzle_temp\":215,\"nozzle_target\":215,"
            "\"bed_temp\":60,\"bed_target\":60,\"error\":\"\"}",
            i, i, (i + 1) * 15, i, (i + 1) * 600);
    }
    pos += snprintf(json + pos, sizeof(json) - pos, "]}");

    result = ParsePrintersResponse(json, pos, &list);
    ASSERT(result == 0);
    ASSERT(list.count == 6);
    ASSERT(strcmp(list.printers[0].id, "p0") == 0);
    ASSERT(strcmp(list.printers[5].id, "p5") == 0);
    ASSERT(list.printers[2].progress == 45);
    PASS();
}

int main(void) {
    printf("test_json:\n");
    test_parse_two_printers();
    test_parse_empty();
    test_parse_single_printer();
    test_parse_malformed();
    test_parse_large_response();
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
