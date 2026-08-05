#ifndef HCI_HANDLER_H
#define HCI_HANDLER_H
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <stdint.h>

#define OGF_DEVICE_INFORMATION         0x04
#define OCF_GET_VERSION                0x0001
#define PAYLOAD_LENGTH                 25
#define PAYLOAD_NULL_BUFFER            NULL

typedef struct __attribute__((packed)){
    uint16_t handle;
    uint16_t conn_interval_min;
    uint16_t conn_interval_max;
    uint16_t conn_latency;
    uint16_t supervision_timeout;
    uint16_t min_ce_length;
    uint16_t max_ce_length;

} le_conn_update_cp;


int open_hci(void);

int hci_update_conn_params(int sock,
                           le_conn_update_cp *params,
                           uint16_t conn_handle,
                           uint16_t conn_interval_min,
                           uint16_t conn_interval_max,
                           uint16_t conn_latency,
                           uint16_t supervision_timeout,
                           uint16_t min_ce_length,
                           uint16_t max_ce_length);

int wait_le_conn_update_complete(int sock);

int get_hci_version(int socket, uint8_t* buffer, int size);

#endif
