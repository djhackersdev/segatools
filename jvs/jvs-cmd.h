#pragma once

enum {
    JVS_CMD_READ_ID = 0x10,
    JVS_CMD_GET_CMD_VERSION = 0x11,
    JVS_CMD_GET_JVS_VERSION = 0x12,
    JVS_CMD_GET_COMM_VERSION = 0x13,
    JVS_CMD_GET_FEATURES = 0x14,
    JVS_CMD_READ_SWITCHES = 0x20,
    JVS_CMD_READ_COIN = 0x21,
    JVS_CMD_READ_ANALOGS = 0x22,
    JVS_CMD_WRITE_GPIO = 0x32,
    JVS_CMD_RESET = 0xF0,
    JVS_CMD_ASSIGN_ADDR = 0xF1,
};

#pragma pack(push, 1)

struct jvs_req_read_switches {
    uint8_t cmd;
    uint8_t num_players;
    uint8_t bytes_per_player;
};

struct jvs_req_read_coin {
    uint8_t cmd;
    uint8_t nslots;
};

struct jvs_req_read_analogs {
    uint8_t cmd;
    uint8_t nanalogs;
};

struct jvs_req_reset {
    uint8_t cmd;
    uint8_t unknown;
};

struct jvs_req_assign_addr {
    uint8_t cmd;
    uint8_t addr;
};

#pragma pack(pop)
