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

static WindowPtr gWindow;
static Boolean   gQuit = false;

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

static void DrawContent(WindowPtr window) {
    GrafPtr savePort;
    Rect    r;

    GetPort(&savePort);
    SetPort(window);
    r = window->portRect;
    EraseRect(&r);

    TextFont(3); /* Geneva */
    TextSize(12);
    TextFace(bold);
    MoveTo(10, 25);
    DrawString("\pPrintWatch");

    TextSize(9);
    TextFace(0);
    MoveTo(10, 40);
    DrawString("\p3D Printer Monitor for Macintosh");

    MoveTo(10, 65);
    DrawString("\pWaiting for printer data...");

    /* Draw a separator line */
    MoveTo(10, 50);
    LineTo(r.right - 10, 50);

    SetPort(savePort);
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
                /* Refresh will trigger a poll once networking is wired up */
                SetPort(gWindow);
                EraseRect(&gWindow->portRect);
                DrawContent(gWindow);
            } else if (menuItem == kFileQuit) {
                gQuit = true;
            }
            break;

        case kMenuOptions:
            /* Dialogs deferred to Phase 6 */
            break;
    }

    HiliteMenu(0);
}

static void HandleMouseDown(EventRecord *event) {
    WindowPtr window;
    short     part;

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
    }
}

static void HandleUpdate(EventRecord *event) {
    WindowPtr window = (WindowPtr)event->message;

    BeginUpdate(window);
    DrawContent(window);
    EndUpdate(window);
}

int main(void) {
    EventRecord event;

    InitToolbox();
    SetupMenus();

    gWindow = GetNewWindow(128, NULL, (WindowPtr)-1);
    SetPort(gWindow);

    while (!gQuit) {
        if (WaitNextEvent(everyEvent, &event, 30, NULL)) {
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
