hooklib_lib = static_library(
    'hooklib',
    include_directories : inc,
    implicit_include_directories : false,
    c_pch : '../precompiled.h',
    dependencies : [
        capnhook.get_variable('hook_dep'),
    ],
    sources : [
        'clock.c',
        'clock.h',
        'fdshark.c',
        'fdshark.h',
        'gfx.c',
        'gfx.h',
        'setupapi.c',
        'setupapi.h',
        'spike.c',
        'spike.h',
    ],
)
