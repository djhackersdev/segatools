util_lib = static_library(
    'util',
    include_directories : inc,
    implicit_include_directories : false,
    c_pch : '../precompiled.h',
    dependencies : [
        capnhook.get_variable('hook_dep'),
    ],
    sources : [
        'async.c',
        'async.h',
        'crc.c',
        'crc.h',
        'dll-bind.c',
        'dll-bind.h',
        'dprintf.c',
        'dprintf.h',
        'dump.c',
        'dump.h',
        'str.c',
        'str.h',
    ],
)
