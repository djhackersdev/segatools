board_lib = static_library(
    'board',
    include_directories : inc,
    implicit_include_directories : false,
    c_pch : '../precompiled.h',
    dependencies : [
        capnhook.get_variable('hook_dep'),
    ],
    sources : [
        'io3.c',
        'io3.h',
        'sg-frame.c',
        'sg-frame.h',
    ],
)
