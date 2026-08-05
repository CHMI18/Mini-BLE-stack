#ifndef ATT_HANDLER_H
#define ATT_HANDLER_H

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include "acl_handler.h"

#define READ_REQ_OP             0x0A
#define SERVICE_HANDLER_UUID    0x2800
#define CHARACTERISTIC_UUID     0x2803

typedef struct __attribute__((packed)){
    uint16_t start_handle;
    uint16_t end_handle;
    uint8_t uuid[128];
    uint8_t uuid_len;
    uint8_t n_characteristics;
} gatt_service_entry;

typedef struct __attribute__((packed)){
    uint16_t decl_handle;
    uint8_t properties;
    uint16_t value_handle;
    uint8_t uuid[128];
    uint8_t uuid_len;

} gatt_characteristic_entry;

typedef struct __attribute__((packed)){
    uint8_t format;
    uint16_t handle;
    uint8_t uuid_len;
    uint8_t uuid[128];
} gatt_information_entry;



typedef struct {
    uint8_t opcode;
    union {
        struct { uint8_t req_opcode; uint16_t handle; uint8_t error_code; } error;
        struct { uint8_t value[128]; uint16_t len; } read_resp;
        struct { gatt_service_entry services[128]; uint8_t count; } group_type_resp;
        struct { gatt_characteristic_entry characteristics[128]; uint8_t count; } read_by_type_resp;
        struct { gatt_information_entry descriptor_info[128]; uint8_t count; } find_information_resp;
    } data;
} att_parsed_response;

int parse_att_response(att_pdu_response *msg,
                       att_parsed_response *out);

void att_build_read_req(uint8_t *out_buf,
                       uint16_t service_handle);

void att_build_write_req(uint16_t handle,
                         uint8_t *value,
                         uint16_t value_len,
                         uint8_t *out_buf);

void att_build_read_by_group_type_req(uint8_t *buf,
                                      uint16_t start_handle,
                                      uint16_t end_handle);

void att_build_find_information_req(uint8_t *buf,
                                    uint16_t start_handle,
                                    uint16_t end_handle);

int read_characteristic(int att_sock,
                         uint16_t handle);

int write_characteristic(int att_sock,
                         uint16_t handle,
                         uint8_t *data,
                         uint16_t size);


int discover_all_services(int att_sock);

void get_available_services(void);

void att_build_read_by_type_req(uint8_t *buf,
                                uint16_t start_handle,
                                uint16_t end_handle);

int discover_all_characteristics(int att_sock);

static const char *uuid16_to_string(uint16_t uuid);

static void print_properties(uint8_t properties);

void get_characteristics(void);
#endif
