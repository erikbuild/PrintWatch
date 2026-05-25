cmake_minimum_required(VERSION 3.9)
project(PrintWatch C)

add_application(PrintWatch
    resources/PrintWatch.r
    src/main.c
    src/printers.c
    src/json.c
)

set_target_properties(PrintWatch PROPERTIES
    COMPILE_OPTIONS "-Os;-ffunction-sections"
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
