/* ABOUTME: Entry point for PrintWatch, a 3D printer monitor for Mac System 7.
   ABOUTME: Initializes the Toolbox, sets up menus and window, runs the event loop. */

#include <Quickdraw.h>
#include <Fonts.h>
#include <Windows.h>
#include <Menus.h>
#include <TextEdit.h>
#include <Dialogs.h>
#include <Events.h>
#include <Devices.h>
#include <ToolUtils.h>
#include <MacMemory.h>
#include <SegLoad.h>
#include <Resources.h>
#include <string.h>
#include <stdio.h>

#include "printers.h"
#include "ui.h"
#include "config.h"

enum {
    kMenuApple   = 128,
    kMenuFile    = 129,
    kMenuOptions = 130
};

enum {
    kAppleAbout = 1
};

enum {
    kFileRefresh = 1,
    /* separator = 2 */
    kFileQuit    = 3
};

static WindowPtr   gWindow;
static Boolean      gQuit = false;
static int          gCurrentView = kViewList;
static int          gSelectedPrinter = 0;
static PrinterList  gPrinterList;
static AppConfig    gConfig;
static unsigned long gNextPollTime = 0;
static char         gStatusMessage[80];

static void InitToolbox(void) {
    InitGraf(&qd.thePort);
    InitFonts();
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs(NULL);
    InitCursor();
    FlushEvents(everyEvent, 0);
}

static void SetupMenus(void) {
    Handle menuBar = GetNewMBar(128);
    SetMenuBar(menuBar);
    DisposeHandle(menuBar);
    AppendResMenu(GetMenuHandle(kMenuApple), 'DRVR');
    DrawMenuBar();
}

static void LoadTestData(void) {
    PrinterList_Init(&gPrinterList);
    gPrinterList.count = 4;

    strcpy(gPrinterList.printers[0].id, "mk4");
    strcpy(gPrinterList.printers[0].name, "Prusa MK4");
    strcpy(gPrinterList.printers[0].type, "prusalink");
    strcpy(gPrinterList.printers[0].state, "printing");
    gPrinterList.printers[0].progress = 78;
    strcpy(gPrinterList.printers[0].job, "benchy.gcode");
    gPrinterList.printers[0].time_remaining = 4320;
    gPrinterList.printers[0].nozzle_temp = 215;
    gPrinterList.printers[0].nozzle_target = 215;
    gPrinterList.printers[0].bed_temp = 60;
    gPrinterList.printers[0].bed_target = 60;

    strcpy(gPrinterList.printers[1].id, "voron");
    strcpy(gPrinterList.printers[1].name, "Voron 2.4");
    strcpy(gPrinterList.printers[1].type, "moonraker");
    strcpy(gPrinterList.printers[1].state, "idle");
    gPrinterList.printers[1].nozzle_temp = 22;
    gPrinterList.printers[1].bed_temp = 21;

    strcpy(gPrinterList.printers[2].id, "ender");
    strcpy(gPrinterList.printers[2].name, "Ender 3 K1");
    strcpy(gPrinterList.printers[2].type, "moonraker");
    strcpy(gPrinterList.printers[2].state, "printing");
    gPrinterList.printers[2].progress = 18;
    strcpy(gPrinterList.printers[2].job, "bracket.gcode");
    gPrinterList.printers[2].time_remaining = 16200;
    gPrinterList.printers[2].nozzle_temp = 210;
    gPrinterList.printers[2].nozzle_target = 210;
    gPrinterList.printers[2].bed_temp = 60;
    gPrinterList.printers[2].bed_target = 60;

    strcpy(gPrinterList.printers[3].id, "xl");
    strcpy(gPrinterList.printers[3].name, "Prusa XL");
    strcpy(gPrinterList.printers[3].type, "prusalink");
    strcpy(gPrinterList.printers[3].state, "attention");
    strcpy(gPrinterList.printers[3].error, "Filament change needed");
    gPrinterList.printers[3].nozzle_temp = 215;
    gPrinterList.printers[3].nozzle_target = 215;
    gPrinterList.printers[3].bed_temp = 60;
    gPrinterList.printers[3].bed_target = 60;
}

static void DrawWindow(WindowPtr window) {
    if (gCurrentView == kViewList) {
        UI_DrawListView(window, &gPrinterList, gSelectedPrinter);
    } else {
        UI_DrawDetailView(window, &gPrinterList.printers[gSelectedPrinter]);
    }
    UI_DrawStatusBar(window, gStatusMessage);
}

static void HandleMenuChoice(long menuChoice) {
    short menuID   = HiWord(menuChoice);
    short menuItem = LoWord(menuChoice);
    Str255 daName;

    switch (menuID) {
        case kMenuApple:
            if (menuItem == kAppleAbout) {
                Alert(130, NULL);
            } else {
                GetMenuItemText(GetMenuHandle(kMenuApple), menuItem, daName);
                OpenDeskAcc(daName);
            }
            break;

        case kMenuFile:
            if (menuItem == kFileRefresh) {
                gNextPollTime = 0;
                strcpy(gStatusMessage, "Refreshing...");
                UI_DrawStatusBar(gWindow, gStatusMessage);
            } else if (menuItem == kFileQuit) {
                gQuit = true;
            }
            break;

        case kMenuOptions:
            break;
    }

    HiliteMenu(0);
}

static void HandleContentClick(Point localPt) {
    if (gCurrentView == kViewList) {
        int row = UI_HitTestRow(localPt.v, gPrinterList.count);
        if (row >= 0) {
            if (row == gSelectedPrinter) {
                /* Double-click-like: open detail */
                gCurrentView = kViewDetail;
                UI_InvalidateAll(gWindow);
            } else {
                int oldSel = gSelectedPrinter;
                gSelectedPrinter = row;
                UI_InvalidateRow(gWindow, oldSel);
                UI_InvalidateRow(gWindow, gSelectedPrinter);
            }
        }
    } else {
        /* In detail view, clicking in the bottom area goes back */
        Rect r = gWindow->portRect;
        if (localPt.v > r.bottom - kStatusBarHeight - 20) {
            gCurrentView = kViewList;
            UI_InvalidateAll(gWindow);
        }
    }
}

static void HandleMouseDown(EventRecord *event) {
    WindowPtr window;
    short     part;
    Point     localPt;

    part = FindWindow(event->where, &window);

    switch (part) {
        case inMenuBar:
            HandleMenuChoice(MenuSelect(event->where));
            break;

        case inDrag:
            DragWindow(window, event->where, &qd.screenBits.bounds);
            break;

        case inGoAway:
            if (TrackGoAway(window, event->where)) {
                gQuit = true;
            }
            break;

        case inContent:
            if (window != FrontWindow()) {
                SelectWindow(window);
            } else {
                localPt = event->where;
                GlobalToLocal(&localPt);
                HandleContentClick(localPt);
            }
            break;

        case inSysWindow:
            SystemClick(event, window);
            break;
    }
}

static void HandleKeyDown(EventRecord *event) {
    char key = event->message & charCodeMask;

    if (event->modifiers & cmdKey) {
        HandleMenuChoice(MenuKey(key));
        return;
    }

    if (gCurrentView == kViewList) {
        switch (key) {
            case 0x1E: /* up arrow */
                if (gSelectedPrinter > 0) {
                    int oldSel = gSelectedPrinter;
                    gSelectedPrinter--;
                    UI_InvalidateRow(gWindow, oldSel);
                    UI_InvalidateRow(gWindow, gSelectedPrinter);
                }
                break;
            case 0x1F: /* down arrow */
                if (gSelectedPrinter < gPrinterList.count - 1) {
                    int oldSel = gSelectedPrinter;
                    gSelectedPrinter++;
                    UI_InvalidateRow(gWindow, oldSel);
                    UI_InvalidateRow(gWindow, gSelectedPrinter);
                }
                break;
            case 0x0D: /* return */
            case 0x03: /* enter */
                gCurrentView = kViewDetail;
                UI_InvalidateAll(gWindow);
                break;
        }
    } else {
        if (key == 0x1B) { /* escape */
            gCurrentView = kViewList;
            UI_InvalidateAll(gWindow);
        }
    }
}

static void HandleUpdate(EventRecord *event) {
    WindowPtr window = (WindowPtr)event->message;

    BeginUpdate(window);
    DrawWindow(window);
    EndUpdate(window);
}

int main(void) {
    EventRecord event;

    InitToolbox();
    SetupMenus();
    Config_Load(&gConfig);

    gWindow = GetNewWindow(128, NULL, (WindowPtr)-1);
    SetPort(gWindow);

    LoadTestData();
    sprintf(gStatusMessage, "Proxy: %s:%d  Poll: %ds",
            gConfig.proxyIP, gConfig.proxyPort, gConfig.pollIntervalSecs);

    while (!gQuit) {
        if (WaitNextEvent(everyEvent, &event, 15, NULL)) {
            switch (event.what) {
                case mouseDown:
                    HandleMouseDown(&event);
                    break;
                case keyDown:
                case autoKey:
                    HandleKeyDown(&event);
                    break;
                case updateEvt:
                    HandleUpdate(&event);
                    break;
                case activateEvt:
                    break;
                case osEvt:
                    break;
            }
        }
    }

    return 0;
}
