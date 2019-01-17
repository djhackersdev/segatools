shared_library(
    'chunihook',
    name_prefix : '',
    include_directories : inc,
    implicit_include_directories : false,
    c_pch : [
        '../precompiled.c',
        '../precompiled.h',
    ],
    dependencies : [
        capnhook.get_variable('hook_dep'),
        capnhook.get_variable('hooklib_dep'),
    ],
    link_with : [
        board_lib,
        jvs_lib,
        nu_lib,
        util_lib,
    ],
    sources : [
        'dllmain.c',
        'jvs.c',
        'jvs.h',
        'slider-hook.c',
        'slider-hook.h',
    ],
)
