shared_library(
    'divahook',
    name_prefix : '',
    include_directories : inc,
    implicit_include_directories : false,
    vs_module_defs : 'divahook.def',
    c_pch : '../precompiled.h',
    dependencies : [
        capnhook.get_variable('hook_dep'),
        capnhook.get_variable('hooklib_dep'),
    ],
    link_with : [
        aimeio_dll,
        amex_lib,
        board_lib,
        jvs_lib,
        platform_lib,
        util_lib,
    ],
    sources : [
        '_com10.c',
        'dllmain.c',
        'jvs.c',
        'jvs.h',
        'slider-hook.c',
        'slider-hook.h',
    ],
)
