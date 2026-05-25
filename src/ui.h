/* ABOUTME: QuickDraw rendering for printer status views.
   ABOUTME: List view (all printers) and detail view (single printer). */

#ifndef UI_H
#define UI_H

#include <Windows.h>
#include "printers.h"

enum {
    kViewList   = 0,
    kViewDetail = 1
};

#define kRowHeight    46
#define kHeaderHeight 0
#define kStatusBarHeight 18

void UI_DrawListView(WindowPtr window, PrinterList *list, int selectedIndex);
void UI_DrawDetailView(WindowPtr window, PrinterStatus *printer);
void UI_DrawStatusBar(WindowPtr window, const char *message);

/* Returns the printer index at the given local y coordinate, or -1. */
int UI_HitTestRow(int localY, int printerCount);

/* Invalidate a single row for redraw. */
void UI_InvalidateRow(WindowPtr window, int rowIndex);

/* Invalidate the entire content area. */
void UI_InvalidateAll(WindowPtr window);

#endif
