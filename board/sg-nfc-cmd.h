#pragma once

#include <stdint.h>

#pragma pack(push, 1)

enum {
    SG_NFC_CMD_GET_FW_VERSION       = 0x30,
    SG_NFC_CMD_GET_HW_VERSION       = 0x32,
    SG_NFC_CMD_RADIO_ON             = 0x40,
    SG_NFC_CMD_RADIO_OFF            = 0x41,
    SG_NFC_CMD_POLL                 = 0x42,
    SG_NFC_CMD_MIFARE_SELECT_TAG    = 0x43,
    SG_NFC_CMD_MIFARE_SET_KEY_BANA  = 0x50,
    SG_NFC_CMD_MIFARE_READ_BLOCK    = 0x52,
    SG_NFC_CMD_MIFARE_SET_KEY_AIME  = 0x54,
    SG_NFC_CMD_MIFARE_AUTHENTICATE  = 0x55, /* guess based on time sent */
    SG_NFC_CMD_RESET                = 0x62,
    SG_NFC_CMD_FELICA_ENCAP         = 0x71,
};

struct sg_nfc_res_get_fw_version {
    struct sg_res_header res;
    char version[23];
};

struct sg_nfc_res_get_hw_version {
    struct sg_res_header res;
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

struct sg_nfc_poll_mifare {
    uint8_t type;
    uint8_t id_len;
    uint32_t uid;
};

struct sg_nfc_poll_felica {
    uint8_t type;
    uint8_t id_len;
    uint64_t IDm;
    uint64_t PMm;
};

struct sg_nfc_res_poll {
    struct sg_res_header res;
    uint8_t count;
    uint8_t payload[250];
};

struct sg_nfc_req_mifare_select_tag {
    struct sg_res_header res;
    uint32_t uid;
};

struct sg_nfc_req_mifare_read_block {
    struct sg_req_header req;
    struct {
        uint32_t uid;
        uint8_t block_no;
    } payload;
};

struct sg_nfc_res_mifare_read_block {
    struct sg_res_header res;
    uint8_t block[16];
};

struct sg_nfc_req_felica_encap {
    struct sg_req_header req;
    uint64_t IDm;
    uint8_t payload[243];
};

struct sg_nfc_res_felica_encap {
    struct sg_res_header res;
    uint8_t payload[250];
};

union sg_nfc_req_any {
    uint8_t bytes[256];
    struct sg_req_header simple;
    struct sg_nfc_req_mifare_set_key mifare_set_key;
    struct sg_nfc_req_mifare_read_block mifare_read_block;
    struct sg_nfc_req_mifare_50 mifare_50;
    struct sg_nfc_req_poll_40 poll_40;
    struct sg_nfc_req_felica_encap felica_encap;
};

union sg_nfc_res_any {
    uint8_t bytes[256];
    struct sg_res_header simple;
    struct sg_nfc_res_get_fw_version get_fw_version;
    struct sg_nfc_res_get_hw_version get_hw_version;
    struct sg_nfc_res_poll poll;
    struct sg_nfc_res_mifare_read_block mifare_read_block;
    struct sg_nfc_res_felica_encap felica_encap;
};

#pragma pack(pop)
