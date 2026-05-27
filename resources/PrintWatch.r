/* ABOUTME: Rez resource definitions for PrintWatch application.
   ABOUTME: Menus, windows, config strings, alerts, and SIZE resource. */

#include "Types.r"
#include "PrintWatchIcons.r"

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
    { 40, 2, 338, 508 },
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
        "10";
        "0"
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
            "PrintWatch 1.2\r"
            "3D Printer Monitor\r"
            "for Mac System 7 with MacTCP\r"
            "Created by Erik Reynolds (@erikbuild) in 2026.\r"
        };
    }
};

/* Set Proxy Address dialog */
resource 'DLOG' (131) {
    { 80, 80, 210, 432 },
    dBoxProc,
    visible,
    noGoAway,
    0,
    131,
    "Set Proxy Address",
    centerMainScreen
};

resource 'DITL' (131) {
    {
        { 95, 250, 115, 330 },
        Button { enabled, "OK" };

        { 95, 155, 115, 235 },
        Button { enabled, "Cancel" };

        { 10, 15, 26, 130 },
        StaticText { enabled, "Proxy IP:" };

        { 10, 135, 26, 335 },
        EditText { enabled, "" };

        { 38, 15, 54, 130 },
        StaticText { enabled, "Port:" };

        { 38, 135, 54, 210 },
        EditText { enabled, "" };

        { 70, 15, 86, 335 },
        StaticText { enabled, "Changes take effect on next poll." };
    }
};

/* Set Poll Interval dialog */
resource 'DLOG' (132) {
    { 80, 100, 190, 412 },
    dBoxProc,
    visible,
    noGoAway,
    0,
    132,
    "Set Poll Interval",
    centerMainScreen
};

resource 'DITL' (132) {
    {
        { 75, 220, 95, 290 },
        Button { enabled, "OK" };

        { 75, 125, 95, 205 },
        Button { enabled, "Cancel" };

        { 10, 15, 26, 170 },
        StaticText { enabled, "Interval (seconds):" };

        { 10, 175, 26, 240 },
        EditText { enabled, "" };

        { 38, 15, 54, 290 },
        StaticText { enabled, "How often to poll the proxy." };
    }
};

/* Application icon */
resource 'ICN#' (128) {
    {
        $"0000 7FE0"
        $"0000 8010"
        $"0001 0008"
        $"1FFF FFF8"
        $"1FFF FFF8"
        $"1FFF FFF8"
        $"1C01 0038"
        $"1C00 8038"
        $"1C00 8038"
        $"1C03 C038"
        $"1C03 C038"
        $"1FFF FFF8"
        $"1E03 C078"
        $"1E03 C078"
        $"1C01 8038"
        $"1C00 0038"
        $"1C00 0038"
        $"1C00 0038"
        $"1C00 0038"
        $"1C7F FE38"
        $"1CC0 0338"
        $"1C80 0138"
        $"1FFF FFF8"
        $"1CE0 0738"
        $"1CE0 0738"
        $"3FFF FFFC"
        $"2000 0204"
        $"200F F504"
        $"2008 1504"
        $"200F F204"
        $"3FFF FFFC"
        $"1800 0018",

        $"0000 7FE0"
        $"0000 FFF0"
        $"0001 FFF8"
        $"1FFF FFF8"
        $"1FFF FFF8"
        $"1FFF FFF8"
        $"1FFF FFF8"
        $"1FFF FFF8"
        $"1FFF FFF8"
        $"1FFF FFF8"
        $"1FFF FFF8"
        $"1FFF FFF8"
        $"1FFF FFF8"
        $"1FFF FFF8"
        $"1FFF FFF8"
        $"1FFF FFF8"
        $"1FFF FFF8"
        $"1FFF FFF8"
        $"1FFF FFF8"
        $"1FFF FFF8"
        $"1FFF FFF8"
        $"1FFF FFF8"
        $"1FFF FFF8"
        $"1FFF FFF8"
        $"1FFF FFF8"
        $"3FFF FFFC"
        $"3FFF FFFC"
        $"3FFF FFFC"
        $"3FFF FFFC"
        $"3FFF FFFC"
        $"3FFF FFFC"
        $"1FFF FFF8"
    }
};

/* File reference: APPL type uses icon 128 */
resource 'FREF' (128) {
    'APPL', 0, ""
};

/* Bundle: ties creator code to icons */
resource 'BNDL' (128) {
    'PWch', 0,
    {
        'ICN#', { 0, 128 },
        'FREF', { 0, 128 }
    }
};

/* Owner resource for Finder to associate the bundle */
data 'PWch' (0, "Owner resource") {
    $"00"
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
