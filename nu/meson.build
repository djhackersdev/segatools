nu_lib = static_library(
    'nu',
    include_directories : inc,
    implicit_include_directories : false,
    c_pch : '../precompiled.h',
    dependencies : [
        capnhook.get_variable('hook_dep'),
        capnhook.get_variable('hooklib_dep'),
    ],
    sources : [
        'eeprom.c',
        'eeprom.h',
        'guid.c',
        'nvram.c',
        'nvram.h',
    ],
)
