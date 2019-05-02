amex_lib = static_library(
    'amex',
    include_directories : inc,
    implicit_include_directories : false,
    c_pch : '../precompiled.h',
    dependencies : [
        capnhook.get_variable('hook_dep'),
    ],
    sources : [
        'ds.c',
        'ds.h',
        'eeprom.c',
        'eeprom.h',
        'gpio.c',
        'gpio.h',
        'guid.c',
        'jvs.c',
        'jvs.h',
        'nvram.c',
        'nvram.h',
        'sram.c',
        'sram.h',
    ],
)
