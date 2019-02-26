board_lib = static_library(
    'board',
    include_directories : inc,
    implicit_include_directories : false,
    c_pch : [
        '../precompiled.c',
        '../precompiled.h',
    ],
    dependencies : [
        capnhook.get_variable('hook_dep'),
    ],
    sources : [
        'io3.c',
        'io3.h',
        'sg-cmd.c',
        'sg-cmd.h',
        'sg-frame.c',
        'sg-frame.h',
        'sg-led.c',
        'sg-led.h',
        'sg-led-cmd.h',
        'sg-nfc.c',
        'sg-nfc.h',
        'sg-nfc-cmd.h',
        'slider-cmd.h',
        'slider-frame.c',
        'slider-frame.h',
    ],
)
