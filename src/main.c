#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include "hci_handler.h"
#include "acl_handler.h"
#include "att_handler.h"

#define MAC_ADDRESS "C4:E0:5A:37:6E:8D"

int main(void)
{
    int att_sock = open_att(MAC_ADDRESS, BDADDR_LE_RANDOM);
    if (att_sock < 0) {
        printf("Failed to connect to Nordic\n");
        return -1;
    }

    printf("Connected to Nordic\n");

    uint16_t conn_handle;
    if (get_conn_handle(att_sock, &conn_handle) < 0) {
        printf("Failed to get connection handle\n");
        close(att_sock);
        return -1;
    }

    printf("Connection handle: 0x%04X\n", conn_handle);

    int hci_sock = open_hci();
    if (hci_sock < 0) {
        printf("Failed to open HCI socket\n");
        close(att_sock);
        return -1;
    }

    le_conn_update_cp update_params;
    hci_update_conn_params(hci_sock,
                           &update_params,
                           conn_handle,
                           24,
                           40,
                           0,
                           400,
                           0,
                           0);

    wait_le_conn_update_complete(hci_sock);

    discover_all_services(att_sock);

    discover_all_characteristics(att_sock);

    get_characteristics();


    int service_handle;

    printf("Select a handle: ");
    scanf("%d", &service_handle);

    uint16_t hand = (uint16_t) service_handle;

    for (int i = 0; i < 10; i++){
        read_characteristic(att_sock, hand);
        usleep(50000);
    }

    close(att_sock);
    close(hci_sock);

    return 0;
}

