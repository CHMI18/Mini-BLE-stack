#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>
#include "acl_handler.h"

int open_att(const char *mac_address, uint8_t peer_addr_type)
{
    int sock = socket(AF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP); //Defines ATT socket
    if (sock < 0) {                                                 //AF_BLUETOOTH = bluetooth socket
        perror("open_att: socket");                                 //SOCK_SEQPACKET = specific protocol
        return -1;                                                  //BTPROTO_L2CAP = l2cap type messages
    }

    struct sockaddr_l2 local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.l2_family = AF_BLUETOOTH;
    local_addr.l2_cid = htobs(ATT_CID);
    bacpy(&local_addr.l2_bdaddr, BDADDR_ANY);
    local_addr.l2_bdaddr_type = BDADDR_LE_PUBLIC;

    if (bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) { //Binds host end to sock
        perror("open_att: bind");
        close(sock);
        return -1;
    }

    struct sockaddr_l2 remote_addr;
    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.l2_family = AF_BLUETOOTH;
    remote_addr.l2_cid = htobs(ATT_CID);
    str2ba(mac_address, &remote_addr.l2_bdaddr);
    remote_addr.l2_bdaddr_type = peer_addr_type;

    if (connect(sock, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) < 0) { //Socket connects to client end
        perror("open_att: connect");
        close(sock);
        return -1;
    }

    return sock;
}

int get_conn_handle(int att_sock, uint16_t *conn_handle)
{
    struct l2cap_conninfo info;
    socklen_t len = sizeof(info);

    if (getsockopt(att_sock, SOL_L2CAP, L2CAP_CONNINFO, &info, &len) < 0) {
        perror("get_conn_handle: getsockopt");
        return -1;
    }

    *conn_handle = info.hci_handle;
    return 0;
}

void send_att_pdu(int att_sock, uint16_t size, uint8_t *data) //write to socket + error message
{
    int n = write(att_sock, data, size);
    if (n < 0) {
        perror("send_att_pdu: write");
    }
}

int read_att_pdu_response(int att_sock, uint16_t size, uint8_t *buf) //Read from socket + error message
{
    int n = read(att_sock, buf, size);
    if (n < 0) {
        perror("read_att_pdu_response: read");
    }
    return n;
}

void decode_pdu_msg(uint8_t *buf, uint16_t len, att_pdu_response *msg) //Remove headers and store rest in buffer
{
    msg->l2cap_size = len;
    for (int i = 0; i < len; i++) {
        msg->buffer[i] = buf[i];
    }
}
