util_lib = static_library(
    'util',
    include_directories : inc,
    implicit_include_directories : false,
    c_pch : '../precompiled.h',
    dependencies : [
        capnhook.get_variable('hook_dep'),
    ],
    sources : [
        'dprintf.c',
        'dprintf.h',
        'setupapi.c',
        'setupapi.h',
    ],
)
