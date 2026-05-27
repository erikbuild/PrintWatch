/* ABOUTME: QuickDraw rendering for the PrintWatch UI.
   ABOUTME: Draws printer list, detail view, progress bars, and status bar. */

#include "ui.h"
#include "printers.h"
#include <Quickdraw.h>
#include <Fonts.h>
#include <TextUtils.h>
#include <string.h>
#include <stdio.h>

#define kFontGeneva 3
#define kMarginLeft 10
#define kMarginRight 10
#define kProgressBarHeight 10

static void DrawCString(const char *s) {
    unsigned char pstr[256];
    int len = strlen(s);
    if (len > 255) len = 255;
    pstr[0] = len;
    memcpy(pstr + 1, s, len);
    DrawString(pstr);
}

static void DrawProgressBar(Rect *bounds, int progress) {
    Rect fill;
    int fillWidth;

    FrameRect(bounds);
    fill = *bounds;
    InsetRect(&fill, 1, 1);
    fillWidth = ((fill.right - fill.left) * progress) / 100;
    fill.right = fill.left + fillWidth;
    if (fillWidth > 0) {
        PaintRect(&fill);
    }
}

static void DrawStateLabel(const char *state, int x, int y) {
    MoveTo(x, y);
    if (strcmp(state, "error") == 0 || strcmp(state, "attention") == 0) {
        TextFace(bold);
        DrawCString("! ");
        TextFace(0);
    }
    DrawCString(state);
}

static void DrawListRow(PrinterStatus *printer, int rowIndex, int selected,
                         int contentWidth) {
    int y = kHeaderHeight + (rowIndex * kRowHeight);
    int textY = y + 15;
    char buf[80];
    Rect progressRect;

    /* Selection indicator */
    if (selected) {
        TextFont(kFontGeneva);
        TextSize(9);
        TextFace(0);
        MoveTo(kMarginLeft - 8, textY);
        DrawChar(0xC6); /* bullet character */
    }

    /* Printer name */
    TextFont(kFontGeneva);
    TextSize(12);
    TextFace(bold);
    MoveTo(kMarginLeft, textY);
    DrawCString(printer->name);
    if (printer->model[0]) {
        TextFace(0);
        DrawCString(" (");
        DrawCString(printer->model);
        DrawCString(")");
    }

    /* State */
    TextSize(9);
    TextFace(0);
    DrawStateLabel(printer->state, kMarginLeft + 160, textY);

    if (PrinterIsActive(printer)) {
        /* Progress bar */
        progressRect.left = kMarginLeft + 240;
        progressRect.top = y + 5;
        progressRect.right = contentWidth - kMarginRight - 50;
        progressRect.bottom = progressRect.top + kProgressBarHeight;
        DrawProgressBar(&progressRect, printer->progress);

        /* Percentage */
        sprintf(buf, "%d%%", printer->progress);
        MoveTo(contentWidth - kMarginRight - 45, textY);
        DrawCString(buf);

        /* Second line: job name and time remaining */
        textY = y + 32;
        TextFont(kFontGeneva);
        TextSize(9);
        TextFace(0);
        MoveTo(kMarginLeft + 20, textY);
        DrawCString(printer->job);

        if (printer->time_remaining > 0) {
            FormatTimeRemaining(printer->time_remaining, buf, sizeof(buf));
            MoveTo(contentWidth - kMarginRight - 80, textY);
            DrawCString(buf);
            DrawCString(" rem");
        }
    } else if (strcmp(printer->state, "error") == 0 && printer->error[0]) {
        textY = y + 32;
        TextFont(kFontGeneva);
        TextSize(9);
        TextFace(0);
        MoveTo(kMarginLeft + 20, textY);
        DrawCString(printer->error);
    } else if (strcmp(printer->state, "finished") == 0) {
        textY = y + 32;
        TextFont(kFontGeneva);
        TextSize(9);
        TextFace(0);
        MoveTo(kMarginLeft + 20, textY);
        DrawCString("Print complete");
    }

    /* Row separator */
    MoveTo(kMarginLeft, y + kRowHeight - 2);
    LineTo(contentWidth - kMarginRight, y + kRowHeight - 2);
}

void UI_DrawListView(WindowPtr window, PrinterList *list, int selectedIndex) {
    Rect r;
    int i, contentWidth;

    SetPort(window);
    r = window->portRect;
    contentWidth = r.right - r.left;

    /* Clear content area */
    r.bottom -= kStatusBarHeight;
    EraseRect(&r);

    if (list->count == 0) {
        TextFont(kFontGeneva);
        TextSize(9);
        TextFace(0);
        MoveTo(kMarginLeft, 25);
        DrawCString("No printers configured.");
        return;
    }

    for (i = 0; i < list->count; i++) {
        DrawListRow(&list->printers[i], i, i == selectedIndex, contentWidth);
    }
}

void UI_DrawDetailView(WindowPtr window, PrinterStatus *printer) {
    Rect r, progressRect;
    int contentWidth, y;
    char buf[80];

    SetPort(window);
    r = window->portRect;
    contentWidth = r.right - r.left;

    /* Clear content area */
    r.bottom -= kStatusBarHeight;
    EraseRect(&r);

    y = 20;

    /* Printer name */
    TextFont(kFontGeneva);
    TextSize(12);
    TextFace(bold);
    MoveTo(kMarginLeft, y);
    DrawCString(printer->name);
    if (printer->model[0]) {
        TextFace(0);
        DrawCString(" (");
        DrawCString(printer->model);
        DrawCString(")");
    }
    y += 22;

    /* Separator */
    MoveTo(kMarginLeft, y);
    LineTo(contentWidth - kMarginRight, y);
    y += 16;

    /* State */
    TextFont(kFontGeneva);
    TextSize(10);
    TextFace(0);
    MoveTo(kMarginLeft, y);
    DrawCString("State:    ");
    TextFace(bold);
    DrawCString(printer->state);
    TextFace(0);
    y += 18;

    /* Job */
    if (printer->job[0]) {
        MoveTo(kMarginLeft, y);
        DrawCString("Job:      ");
        DrawCString(printer->job);
        y += 18;
    }

    /* Progress bar (wide) */
    if (PrinterIsActive(printer)) {
        MoveTo(kMarginLeft, y);
        DrawCString("Progress:");
        progressRect.left = kMarginLeft + 80;
        progressRect.top = y - 10;
        progressRect.right = contentWidth - kMarginRight - 60;
        progressRect.bottom = progressRect.top + 14;
        DrawProgressBar(&progressRect, printer->progress);
        sprintf(buf, " %d%%", printer->progress);
        MoveTo(contentWidth - kMarginRight - 55, y);
        DrawCString(buf);
        y += 22;

        /* Time remaining */
        if (printer->time_remaining > 0) {
            MoveTo(kMarginLeft, y);
            DrawCString("Remaining:");
            FormatTimeRemaining(printer->time_remaining, buf, sizeof(buf));
            MoveTo(kMarginLeft + 80, y);
            DrawCString(buf);
            y += 18;
        }
    }

    y += 8;

    /* Temperature section */
    MoveTo(kMarginLeft, y);
    TextFace(bold);
    DrawCString("Temperatures");
    TextFace(0);
    y += 4;
    MoveTo(kMarginLeft, y);
    LineTo(contentWidth - kMarginRight, y);
    y += 16;

    /* Nozzle */
    MoveTo(kMarginLeft, y);
    DrawCString("Nozzle:   ");
    sprintf(buf, "%dC / %dC", printer->nozzle_temp, printer->nozzle_target);
    DrawCString(buf);
    y += 18;

    /* Bed */
    MoveTo(kMarginLeft, y);
    DrawCString("Bed:      ");
    sprintf(buf, "%dC / %dC", printer->bed_temp, printer->bed_target);
    DrawCString(buf);
    y += 18;

    /* Error message */
    if (printer->error[0]) {
        y += 8;
        MoveTo(kMarginLeft, y);
        TextFace(bold);
        DrawCString("Error: ");
        TextFace(0);
        DrawCString(printer->error);
    }

    /* Back hint */
    y = window->portRect.bottom - kStatusBarHeight - 10;
    TextFont(kFontGeneva);
    TextSize(9);
    TextFace(0);
    MoveTo(kMarginLeft, y);
    DrawCString("Press Esc or click here to go back");
}

void UI_DrawStatusBar(WindowPtr window, const char *message) {
    Rect r;

    SetPort(window);
    r = window->portRect;
    r.top = r.bottom - kStatusBarHeight;
    EraseRect(&r);

    /* Separator line */
    MoveTo(0, r.top);
    LineTo(r.right, r.top);

    TextFont(kFontGeneva);
    TextSize(9);
    TextFace(0);
    MoveTo(kMarginLeft, r.top + 13);
    DrawCString(message);
}

int UI_HitTestRow(int localY, int printerCount) {
    int row;
    if (localY < kHeaderHeight) return -1;
    row = (localY - kHeaderHeight) / kRowHeight;
    if (row >= printerCount) return -1;
    return row;
}

void UI_InvalidateRow(WindowPtr window, int rowIndex) {
    Rect r;
    SetPort(window);
    r = window->portRect;
    r.top = kHeaderHeight + (rowIndex * kRowHeight);
    r.bottom = r.top + kRowHeight;
    InvalRect(&r);
}

void UI_InvalidateAll(WindowPtr window) {
    Rect r;
    SetPort(window);
    r = window->portRect;
    InvalRect(&r);
}
