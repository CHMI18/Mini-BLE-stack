#ifndef ACL_HANDLER_H
#define ACL_HANDLER_H

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include <stdint.h>

#define ATT_CID                 0x0004

#define ATT_OP_WRITE_REQ        0x12
#define ATT_OP_WRITE_RESP       0x13
#define ATT_OP_WRITE_CMD        0x52

#define ATT_OP_HANDLE_NOTIFY    0x1B
#define ATT_OP_HANDLE_IND       0x1D
#define ATT_OP_HANDLE_CNF       0x1E

typedef struct {
    uint8_t buffer[256];
    uint16_t l2cap_size;
} att_pdu_response;

int open_att(const char *mac_address, uint8_t peer_addr_type);

int get_conn_handle(int att_sock, uint16_t *conn_handle);

void send_att_pdu(int att_sock, uint16_t size, uint8_t *data);

int read_att_pdu_response(int att_sock, uint16_t size, uint8_t *buf);

void decode_pdu_msg(uint8_t *buf, uint16_t len, att_pdu_response *msg);

#endif
