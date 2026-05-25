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
#include "json.h"
#include "ui.h"
#include "config.h"
#include "network.h"

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

static WindowPtr    gWindow;
static Boolean      gQuit = false;
static int          gCurrentView = kViewList;
static int          gSelectedPrinter = 0;
static PrinterList  gPrinterList;
static AppConfig    gConfig;
static NetState     gNet;
static Boolean      gNetAvailable = false;
static unsigned long gNextPollTime = 0;
static unsigned long gProxyIP = 0;
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

static void InitNetworking(void) {
    int err;

    gProxyIP = Net_ParseIP(gConfig.proxyIP);
    if (gProxyIP == 0) {
        strcpy(gStatusMessage, "Bad proxy IP address");
        return;
    }

    err = Net_Init(&gNet);
    if (err != kNetOK) {
        strcpy(gStatusMessage, "MacTCP not available");
        return;
    }
    gNetAvailable = true;
}

static void PollPrinters(void) {
    char *body;
    int bodyLen;
    int err;
    PrinterList newList;

    if (!gNetAvailable) {
        return;
    }

    strcpy(gStatusMessage, "Polling...");
    UI_DrawStatusBar(gWindow, gStatusMessage);

    err = Net_HttpGet(&gNet, gProxyIP, (short)gConfig.proxyPort,
                      gConfig.proxyIP, "/printers", &body, &bodyLen);

    if (err != kNetOK) {
        switch (err) {
            case kNetConnectFailed:
                strcpy(gStatusMessage, "Cannot reach proxy");
                break;
            case kNetSendFailed:
                strcpy(gStatusMessage, "Send failed");
                break;
            case kNetRecvFailed:
                strcpy(gStatusMessage, "No response");
                break;
            case kNetBadResponse:
                strcpy(gStatusMessage, "Bad response");
                break;
            default:
                strcpy(gStatusMessage, "Network error");
                break;
        }
        UI_DrawStatusBar(gWindow, gStatusMessage);
        return;
    }

    PrinterList_Init(&newList);
    if (ParsePrintersResponse(body, bodyLen, &newList) == 0) {
        gPrinterList = newList;
        if (gSelectedPrinter >= gPrinterList.count) {
            gSelectedPrinter = 0;
        }
        sprintf(gStatusMessage, "Updated  %d printers  Poll: %ds",
                gPrinterList.count, gConfig.pollIntervalSecs);
        UI_InvalidateAll(gWindow);
    } else {
        strcpy(gStatusMessage, "Bad JSON from proxy");
    }
    UI_DrawStatusBar(gWindow, gStatusMessage);
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

    PrinterList_Init(&gPrinterList);
    sprintf(gStatusMessage, "Connecting to %s:%d...",
            gConfig.proxyIP, gConfig.proxyPort);

    InitNetworking();
    gNextPollTime = 0;

    while (!gQuit) {
        /* Poll when the timer expires */
        if (gNetAvailable && TickCount() >= gNextPollTime) {
            PollPrinters();
            gNextPollTime = TickCount() + ((unsigned long)gConfig.pollIntervalSecs * 60);
        }

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
