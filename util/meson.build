util_lib = static_library(
    'util',
    include_directories : inc,
    implicit_include_directories : false,
    c_pch : '../precompiled.h',
    dependencies : [
        capnhook.get_variable('hook_dep'),
    ],
    sources : [
        'clock.c',
        'clock.h',
        'crc.c',
        'crc.h',
        'dprintf.c',
        'dprintf.h',
        'dump.c',
        'dump.h',
        'setupapi.c',
        'setupapi.h',
    ],
)
