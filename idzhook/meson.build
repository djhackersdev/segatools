shared_library(
    'idzhook',
    name_prefix : '',
    include_directories : inc,
    implicit_include_directories : false,
    vs_module_defs : 'idzhook.def',
    c_pch : '../precompiled.h',
    dependencies : [
        capnhook.get_variable('hook_dep'),
        capnhook.get_variable('hooklib_dep'),
        xinput_lib,
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
    ],
)
