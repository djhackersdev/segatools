#pragma once

#include <stdint.h>

enum {
    SG_NFC_CMD_GET_FW_VERSION       = 0x30,
    SG_NFC_CMD_GET_HW_VERSION       = 0x32,
    SG_NFC_CMD_40_POLL              = 0x40,
    SG_NFC_CMD_41_POLL              = 0x41,
    SG_NFC_CMD_MIFARE_POLL          = 0x42,
    SG_NFC_CMD_MIFARE_SELECT_TAG    = 0x43,
    SG_NFC_CMD_MIFARE_50            = 0x50,
    SG_NFC_CMD_MIFARE_READ_BLOCK    = 0x52,
    SG_NFC_CMD_MIFARE_SET_KEY       = 0x54,
    SG_NFC_CMD_MIFARE_55            = 0x55,
    SG_NFC_CMD_RESET                = 0x62,
};

struct sg_nfc_resp_get_fw_version {
    struct sg_resp_header resp;
    char version[23];
};

struct sg_nfc_resp_get_hw_version {
    struct sg_resp_header resp;
    char version[23];
};

struct sg_nfc_req_mifare_set_key {
    struct sg_req_header req;
    uint8_t key_a[6];
};

struct sg_nfc_req_mifare_50 {
    struct sg_req_header req;
    uint8_t payload[6];
};

struct sg_nfc_req_poll_40 {
    struct sg_req_header req;
    uint8_t payload;
};

struct sg_nfc_resp_mifare_poll {
    struct sg_resp_header resp;
    union {
        uint8_t none;
        uint8_t some[7];
    } payload;
};

struct sg_nfc_req_mifare_select_tag {
    struct sg_resp_header resp;
    uint8_t uid[4];
};

struct sg_nfc_req_mifare_read_block {
    struct sg_req_header req;
    struct {
        uint8_t uid[4];
        uint8_t block_no;
    } payload;
};

struct sg_nfc_resp_mifare_read_block {
    struct sg_resp_header resp;
    uint8_t block[16];
};

union sg_nfc_req_any {
    uint8_t bytes[256];
    struct sg_req_header simple;
    struct sg_nfc_req_mifare_set_key mifare_set_key;
    struct sg_nfc_req_mifare_read_block mifare_read_block;
    struct sg_nfc_req_mifare_50 mifare_50;
    struct sg_nfc_req_poll_40 poll_40;
};

union sg_nfc_resp_any {
    uint8_t bytes[256];
    struct sg_resp_header simple;
    struct sg_nfc_resp_get_fw_version get_fw_version;
    struct sg_nfc_resp_get_hw_version get_hw_version;
    struct sg_nfc_resp_mifare_poll mifare_poll;
    struct sg_nfc_resp_mifare_read_block mifare_read_block;
};
