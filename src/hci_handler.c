#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include "hci_handler.h"


int open_hci(void) //Get hci socket file descriptor
            
{
    int dev_id = hci_get_route(NULL);

    if (dev_id < 0){
        return -1;
    }
    int dev_path = hci_open_dev(dev_id);
    return dev_path;
}

int hci_update_conn_params(int sock,
                           le_conn_update_cp *params,
                           uint16_t conn_handle,
                           uint16_t conn_interval_min,
                           uint16_t conn_interval_max,
                           uint16_t conn_latency,
                           uint16_t supervision_timeout,
                           uint16_t min_ce_length,
                           uint16_t max_ce_length){

    params->handle = htobs(conn_handle); //Set desired parameters
    params->conn_interval_min = htobs(conn_interval_min);
    params->conn_interval_max = htobs(conn_interval_max);
    params->conn_latency = htobs(conn_latency);
    params->supervision_timeout = htobs(supervision_timeout);
    params->min_ce_length = htobs(min_ce_length);
    params->max_ce_length = htobs(max_ce_length);

    int err = hci_send_cmd(sock, //send connection parameter update request to hci socket
                           OGF_LE_CTL,
                           OCF_LE_CONN_UPDATE,
                           sizeof(*params),
                           params);

    return err;
}

int wait_le_conn_update_complete(int sock){
    struct hci_filter filter;

    hci_filter_clear(&filter); 
    hci_filter_set_ptype(HCI_EVENT_PKT, &filter);
    hci_filter_set_event(EVT_LE_META_EVENT, &filter);

    setsockopt(sock, //Set socket filter for HCI event responses
               SOL_HCI,
               HCI_FILTER,
               &filter,
               sizeof(filter));

    uint8_t buf[HCI_MAX_EVENT_SIZE]; //Max response size buffer

    while (1){
        ssize_t n = read(sock, buf, sizeof(buf));

        if (n < 0){
            perror("Error: HCI socket read\n");
            return -1;
        }

        if (buf[3] == EVT_LE_CONN_UPDATE_COMPLETE) break;
    } //Wait for connection complete response 

    evt_le_connection_update_complete *evt =
    (evt_le_connection_update_complete *)&buf[4];   

    if (evt->status != 0){
        printf("Connection update failed, status 0x%02x\n", evt->status);
        return -1;
    }

    printf("Connection updated: interval=%d latency=%d timeout=%d\n",
           btohs(evt->interval), btohs(evt->latency), btohs(evt->supervision_timeout));

    return 0;
}

int get_hci_version(int socket, uint8_t* buffer, int size){ //Gets the current hci_version from socket

    struct hci_filter filter;
    hci_filter_clear(&filter); //Sets socket filter
    hci_filter_set_ptype(HCI_EVENT_PKT, &filter);
    hci_filter_set_event(EVT_CMD_COMPLETE, &filter);


    hci_filter_set_opcode(
        cmd_opcode_pack(
            OGF_DEVICE_INFORMATION,
            OCF_GET_VERSION
        ),
        &filter
    );

    setsockopt(
    socket,
    SOL_HCI,
    HCI_FILTER,
    &filter,
    sizeof(filter)
    );
    hci_send_cmd(
        socket,
        OGF_DEVICE_INFORMATION,
        OCF_GET_VERSION,
        PAYLOAD_LENGTH,
        PAYLOAD_NULL_BUFFER
    );
    int n = 0;
    n = read(socket, buffer, size);
    hci_filter_clear(&filter);
    return n;
};
