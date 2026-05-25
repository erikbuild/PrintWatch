/* ABOUTME: Rez resource definitions for PrintWatch application.
   ABOUTME: Menus, windows, config strings, alerts, and SIZE resource. */

#include "Dialogs.r"
#include "Processes.r"

/* Menu bar containing Apple, File, and Options menus */
resource 'MBAR' (128) {
    { 128, 129, 130 }
};

/* Apple menu */
resource 'MENU' (128) {
    128, textMenuProc, allEnabled, enabled, apple,
    {
        "About PrintWatch\311", noIcon, noKey, noMark, plain;
    }
};

/* File menu */
resource 'MENU' (129) {
    129, textMenuProc, allEnabled, enabled, "File",
    {
        "Refresh Now", noIcon, "R", noMark, plain;
        "-", noIcon, noKey, noMark, plain;
        "Quit", noIcon, "Q", noMark, plain;
    }
};

/* Options menu */
resource 'MENU' (130) {
    130, textMenuProc, allEnabled, enabled, "Options",
    {
        "Set Proxy Address\311", noIcon, noKey, noMark, plain;
        "Set Poll Interval\311", noIcon, noKey, noMark, plain;
    }
};

/* Main window: nearly full screen on Mac SE (512x342) */
resource 'WIND' (128) {
    { 40, 2, 338, 510 },
    noGrowDocProc,
    visible,
    goAway,
    0,
    "PrintWatch",
    noAutoCenter
};

/* Config strings: proxy IP, port, poll interval */
resource 'STR#' (128) {
    {
        "10.0.2.2";
        "8080";
        "15"
    }
};

/* Error alert */
resource 'ALRT' (129) {
    { 80, 100, 180, 412 },
    129,
    {
        OK, visible, silent;
        OK, visible, silent;
        OK, visible, silent;
        OK, visible, silent;
    },
    centerMainScreen
};

resource 'DITL' (129) {
    {
        { 70, 220, 90, 290 },
        Button { enabled, "OK" };

        { 10, 70, 58, 290 },
        StaticText { enabled, "^0" };
    }
};

/* About box alert */
resource 'ALRT' (130) {
    { 60, 80, 180, 432 },
    130,
    {
        OK, visible, silent;
        OK, visible, silent;
        OK, visible, silent;
        OK, visible, silent;
    },
    centerMainScreen
};

resource 'DITL' (130) {
    {
        { 90, 240, 110, 330 },
        Button { enabled, "OK" };

        { 10, 20, 80, 330 },
        StaticText { enabled,
            "PrintWatch 1.0\n"
            "3D Printer Monitor\n\n"
            "for Mac System 7"
        };
    }
};

/* SIZE resource: memory requirements */
resource 'SIZE' (-1) {
    reserved,
    acceptSuspendResumeEvents,
    reserved,
    canBackground,
    doesActivateOnFGSwitch,
    backgroundAndForeground,
    dontGetFrontClicks,
    ignoreChildDiedEvents,
    is32BitCompatible,
    notHighLevelEventAware,
    onlyLocalHLEvents,
    notStationeryAware,
    dontUseTextEditServices,
    reserved,
    reserved,
    reserved,
    200 * 1024,
    200 * 1024
};
