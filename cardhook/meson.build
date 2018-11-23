shared_library(
    'cardhook',
    name_prefix : '',
    include_directories: inc,
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
        util_lib,
    ],
    sources : [
        '_com12.c',
        'dllmain.c',
    ],
)
