/* ABOUTME: Host-side unit tests for the printer data model.
   ABOUTME: Tests init, find-by-id, time formatting, and active state checks. */

#include <stdio.h>
#include <string.h>
#include <assert.h>
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

static void test_init(void) {
    PrinterList list;
    TEST("PrinterList_Init zeros everything");
    PrinterList_Init(&list);
    ASSERT(list.count == 0);
    ASSERT(list.last_update_ticks == 0);
    ASSERT(list.printers[0].progress == 0);
    ASSERT(list.printers[0].id[0] == '\0');
    PASS();
}

static void test_find_by_id(void) {
    PrinterList list;
    PrinterStatus *found;

    TEST("FindById returns matching printer");
    PrinterList_Init(&list);
    list.count = 2;
    strncpy(list.printers[0].id, "mk4", MAX_ID_LEN);
    strncpy(list.printers[1].id, "voron", MAX_ID_LEN);
    found = PrinterList_FindById(&list, "voron");
    ASSERT(found != NULL);
    ASSERT(strcmp(found->id, "voron") == 0);
    PASS();

    TEST("FindById returns NULL for unknown id");
    found = PrinterList_FindById(&list, "nonexistent");
    ASSERT(found == NULL);
    PASS();

    TEST("FindById works on empty list");
    PrinterList_Init(&list);
    found = PrinterList_FindById(&list, "anything");
    ASSERT(found == NULL);
    PASS();
}

static void test_format_time(void) {
    char buf[16];

    TEST("FormatTime: zero seconds");
    FormatTimeRemaining(0, buf, sizeof(buf));
    ASSERT(strcmp(buf, "-") == 0);

    PASS();

    TEST("FormatTime: negative seconds");
    FormatTimeRemaining(-100, buf, sizeof(buf));
    ASSERT(strcmp(buf, "-") == 0);
    PASS();

    TEST("FormatTime: 30 seconds = 0:30");
    FormatTimeRemaining(30, buf, sizeof(buf));
    ASSERT(strcmp(buf, "0:30") == 0);
    PASS();

    TEST("FormatTime: 90 seconds = 1:30");
    FormatTimeRemaining(90, buf, sizeof(buf));
    ASSERT(strcmp(buf, "1:30") == 0);
    PASS();

    TEST("FormatTime: 3600 seconds = 1:00 (hours:mins)");
    FormatTimeRemaining(3600, buf, sizeof(buf));
    ASSERT(strcmp(buf, "1:00") == 0);
    PASS();

    TEST("FormatTime: 4320 seconds = 1:12");
    FormatTimeRemaining(4320, buf, sizeof(buf));
    ASSERT(strcmp(buf, "1:12") == 0);
    PASS();

    TEST("FormatTime: 36000 seconds = 10:00");
    FormatTimeRemaining(36000, buf, sizeof(buf));
    ASSERT(strcmp(buf, "10:00") == 0);
    PASS();
}

static void test_printer_is_active(void) {
    PrinterStatus p;

    TEST("PrinterIsActive: printing is active");
    memset(&p, 0, sizeof(p));
    strncpy(p.state, "printing", MAX_STATE_LEN);
    ASSERT(PrinterIsActive(&p) == 1);
    PASS();

    TEST("PrinterIsActive: paused is active");
    strncpy(p.state, "paused", MAX_STATE_LEN);
    ASSERT(PrinterIsActive(&p) == 1);
    PASS();

    TEST("PrinterIsActive: idle is not active");
    strncpy(p.state, "idle", MAX_STATE_LEN);
    ASSERT(PrinterIsActive(&p) == 0);
    PASS();

    TEST("PrinterIsActive: error is not active");
    strncpy(p.state, "error", MAX_STATE_LEN);
    ASSERT(PrinterIsActive(&p) == 0);
    PASS();
}

int main(void) {
    printf("test_printers:\n");
    test_init();
    test_find_by_id();
    test_format_time();
    test_printer_is_active();
    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
