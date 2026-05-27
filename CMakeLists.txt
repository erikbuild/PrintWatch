cmake_minimum_required(VERSION 3.9)
project(PrintWatch C)

add_application(PrintWatch
    CREATOR "PWch"
    resources/PrintWatch.r
    src/main.c
    src/printers.c
    src/json.c
    src/config.c
    src/ui.c
    src/icons.c
    src/network.c
    src/snapshot.c
    src/TCPRoutines.c
    src/TCPHi.c
)

set_target_properties(PrintWatch PROPERTIES
    COMPILE_OPTIONS "-Os;-ffunction-sections"
)

# MacTCPHelper uses unsigned long / Ptr interchangeably (safe on 68K)
set_source_files_properties(src/TCPRoutines.c src/TCPHi.c PROPERTIES
    COMPILE_FLAGS "-Wno-incompatible-pointer-types -Wno-int-conversion"
)

if(CMAKE_SYSTEM_NAME MATCHES Retro68)
    set_target_properties(PrintWatch PROPERTIES
        LINK_FLAGS "-Wl,-gc-sections -Wl,--mac-strip-macsbug"
    )
else()
    set_target_properties(PrintWatch PROPERTIES
        LINK_FLAGS "-Wl,-gc-sections"
    )
endif()
