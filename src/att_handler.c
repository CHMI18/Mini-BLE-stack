#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include "acl_handler.h"
#include "att_handler.h"

static att_parsed_response services; 
static att_parsed_response characteristics;
static att_parsed_response characteristic_information;
 

int parse_att_response(att_pdu_response *msg, att_parsed_response *out){ //parser for pdu responses

    out->opcode = msg->buffer[0]; 

    switch(out->opcode){
        case 0x01: //Error response
        {
        out->data.error.req_opcode = msg->buffer[1];
        out->data.error.handle = (msg->buffer[3] << 8) | msg->buffer[2];
        out->data.error.error_code = msg->buffer[4];

        if (out->data.error.error_code == 0x0A){return 1;}
        else {return -1;}
        }
        
        case 0x05:
        {
        if (msg->buffer[1] != 0x01 && msg->buffer[1] != 0x02){ 
            printf("Error: Unexpected UUID format\n");
            return -1;
        }

        uint8_t count = out->data.find_information_resp.count;
        uint8_t entry_len = (msg->buffer[1] == 0x01) ? 4 : 18;
        uint8_t uuid_len = entry_len - 2;
        uint8_t entry_count = (msg->l2cap_size - 2)/entry_len;

        for (int i = 0; i < entry_count; i++){
            uint8_t entry_start = 2 + i*entry_len;

            out->data.find_information_resp.descriptor_info[count + i].format = msg->buffer[1];
            out->data.find_information_resp.descriptor_info[count + i].uuid_len = (msg->buffer[1] == 0x01) ? 2 : 16;
            out->data.find_information_resp.descriptor_info[count + i].handle = msg->buffer[entry_start] & 0x00FF|
                                                                             (msg->buffer[entry_start + 1] << 8) & 0xFF00;
            for (int k = 0; k < uuid_len; k++){
                out->data.find_information_resp.descriptor_info[count + i].uuid[k] = msg->buffer[entry_start + 2 + k];
            }     
        }
        out->data.find_information_resp.count += entry_count;

        return 0;
        }

        case 0x09: //Read_by_type response
        {
        uint8_t count = out->data.read_by_type_resp.count;

        uint8_t service_len = msg->buffer[1];
        uint8_t n_characteristics = (msg->l2cap_size - 2)/service_len;

        for (int i = 0; i < n_characteristics; i ++){
        uint8_t entry_start = 2 + i * service_len;

        out->data.read_by_type_resp.characteristics[count + i].uuid_len = service_len - 5;
        out->data.read_by_type_resp.characteristics[count + i].decl_handle = (msg->buffer[entry_start + 1] << 8 & 0xFF00) 
                                                                         | (msg->buffer[entry_start] & 0x00FF);
        out->data.read_by_type_resp.characteristics[count + i].properties = msg->buffer[entry_start + 2];
        out->data.read_by_type_resp.characteristics[count + i].value_handle = (msg->buffer[entry_start + 4] << 8 & 0xFF00) 
                                                                          | (msg->buffer[entry_start + 3] & 0x00FF);
        uint8_t uuid_len = service_len - 5;

        for(int k = 0; k < uuid_len; k ++){
            out->data.read_by_type_resp.characteristics[count + i].uuid[k] = msg->buffer[entry_start + 5 + k];
        }
        
    }
        out->data.read_by_type_resp.count += n_characteristics;
        return 0;
        }

        case 0x0B://Standard pdu response message
        {
        out->data.read_resp.len = msg->l2cap_size - 1;
        for (int i = 0; i < out->data.read_resp.len; i++){
            out->data.read_resp.value[i] = msg->buffer[1 + i];
        }
        return 0;
        }

        case 0x11:
        { // Service description response
        uint8_t service_desc_len = msg->buffer[1];
        uint8_t service_count = (msg->l2cap_size - 2) / service_desc_len;
        uint8_t uuid_len = service_desc_len - 4;
        out->data.group_type_resp.count = service_count;

        for (int i = 0; i < service_count; i += 1){
            int entry_start = 2 + i * service_desc_len;

            out->data.group_type_resp.services[i].start_handle =
            (msg->buffer[entry_start + 1] << 8 & 0xFF00)|
            (msg->buffer[entry_start] & 0x00FF);

            out->data.group_type_resp.services[i].end_handle =
            (msg->buffer[entry_start + 3] << 8 & 0xFF00)|
            (msg->buffer[entry_start + 2] & 0x00FF);

            out->data.group_type_resp.services[i].uuid_len = uuid_len;

            for (int k = 0; k  < uuid_len; k++){
            out->data.group_type_resp.services[i].uuid[k] = msg->buffer[entry_start + service_desc_len - (k + 1)];
            }
        }
        return 0;
        }

        case 0x13:
            return 0;

        default:
        printf("Unhandled opcode: 0x%02x\n", out->opcode);
        return -2;
    }

};

void att_build_read_req(uint8_t *out_buf,
                       uint16_t service_handle){

    out_buf[0] = READ_REQ_OP;
    out_buf[1] = service_handle & 0xFF;
    out_buf[2] = (service_handle >> 8) & 0xFF;

};

void att_build_write_req(uint16_t handle,
                         uint8_t *value,
                         uint16_t value_len,
                         uint8_t *out_buf){
    out_buf[0] = 0x12;
    out_buf[1] = handle & 0xFF;
    out_buf[2] = (handle >> 8) & 0xFF;
    for (int i = 0; i < value_len; i++) {
        out_buf[3 + i] = value[i];
    }
};

void att_build_read_by_type_req(uint8_t *buf,
                                uint16_t start_handle,
                                uint16_t end_handle){
    buf[0] = 0x08;
    buf[1] = start_handle & 0xFF;
    buf[2] = (start_handle >> 8) & 0xFF;
    buf[3] = end_handle & 0xFF;
    buf[4] = (end_handle >> 8) & 0xFF;
    buf[5] = CHARACTERISTIC_UUID & 0xFF;
    buf[6] = (CHARACTERISTIC_UUID >> 8) & 0xFF;
}

void att_build_read_by_group_type_req(uint8_t *buf,
                                      uint16_t start_handle,
                                      uint16_t end_handle){
    buf[0] = 0x10;
    buf[1] = start_handle & 0xFF;
    buf[2] = start_handle >> 8 & 0xFF;
    buf[3] = end_handle & 0xFF;
    buf[4] = end_handle >> 8 & 0xFF;
    buf[5] = SERVICE_HANDLER_UUID & 0xFF;
    buf[6] = SERVICE_HANDLER_UUID >> 8 & 0xFF;
}

void att_build_find_information_req(uint8_t *buf,
                                    uint16_t start_handle,
                                    uint16_t end_handle){
    buf[0] = 0x04;
    buf[1] = start_handle & 0xFF;
    buf[2] = start_handle >> 8 & 0xFF;
    buf[3] = end_handle & 0xFF;
    buf[4] = end_handle >> 8 & 0xFF;

};

int read_characteristic(int att_sock,
                              uint16_t handle){
    uint8_t req[3];
    uint8_t raw_bytes[256];
    att_pdu_response response;
    att_parsed_response parsed;

    int r_bytes;
    int err;

    att_build_read_req(req, handle);
    send_att_pdu(att_sock, sizeof(req), req);

    r_bytes = read_att_pdu_response(att_sock, sizeof(raw_bytes), raw_bytes);

    if (r_bytes <= 0) {
        printf("No data received\n");
        return -1;
    }

    decode_pdu_msg(raw_bytes, r_bytes, &response);
    err = parse_att_response(&response, &parsed);

    if (err == -1) {
        printf("Error response: req_opcode=0x%02x handle=0x%04x error_code=0x%02x\n",
               parsed.data.error.req_opcode, parsed.data.error.handle,
               parsed.data.error.error_code);
        return -1;
    } else if (err == 0 && parsed.opcode == 0x0B) {
        printf("Read succeeded, value: ");
        for (int i = 0; i < parsed.data.read_resp.len; i++) {
            printf("%02X ", parsed.data.read_resp.value[i]);
        }
        printf("\n");
        
    }
    return 0;
}

int write_characteristic(int att_sock,
                          uint16_t handle,
                          uint8_t *data,
                          uint16_t size){
    
    uint8_t req[3 + size];
    uint8_t raw_bytes[256];
    att_pdu_response pdu;
    att_parsed_response out;

    att_build_write_req(handle, data, size, req);
    send_att_pdu(att_sock, size + 3, req);
    
    int r_bytes = read_att_pdu_response(att_sock, sizeof(raw_bytes), raw_bytes);

    if (r_bytes <= 0) {
        printf("No data received\n");
        return -1;
    }

    decode_pdu_msg(raw_bytes, r_bytes, &pdu);
    int err = parse_att_response(&pdu, &out);

    if (err < 0){
    printf("ATT Error 0x%02X\n", out.data.error.error_code);
        return -1;
    }
    else {
        return 0;
    }

    
}

int discover_all_characteristics(int att_sock){

    uint8_t req[7];
    uint8_t raw_bytes[256];
    att_pdu_response pdu;
    att_parsed_response resp;

    characteristics.data.read_by_type_resp.count = 0;
    uint8_t last_discovered = 0;
    uint8_t discovered = 0;

    for (int i = 0; i < services.data.group_type_resp.count; i++){

        uint16_t start_handle = services.data.group_type_resp.services[i].start_handle;
        uint16_t end_handle = services.data.group_type_resp.services[i].end_handle;

        while(1){
    
        att_build_read_by_type_req(req, start_handle, end_handle);
        send_att_pdu(att_sock, sizeof(req), req);

        int n = read_att_pdu_response(att_sock, sizeof(raw_bytes), raw_bytes);
        if (n < 0){
            printf("Error: read pdu response\n");
            return -1;
        }
        
        decode_pdu_msg(raw_bytes, n, &pdu); 
        printf("ATT PDU:\n");

        for (int i = 0; i < pdu.l2cap_size; i++)
        printf("%02X ", pdu.buffer[i]);

        printf("\n");
        int err = parse_att_response(&pdu, &characteristics);
        if (err < 0){
            printf("Error: Parse att response");
            return -1;
        }
        else if (err == 1){break;}

        discovered = characteristics.data.read_by_type_resp.count;
        if (discovered != 0){
        start_handle = characteristics.data.read_by_type_resp.characteristics[discovered - 1].value_handle;
        }
        else {
        start_handle += 1;    
        }
    }
        services.data.group_type_resp.services[i].n_characteristics = discovered - last_discovered;
        last_discovered = discovered;
    
}
    return 0;
}

int discover_characteristic_information(int att_sock)
{
    uint8_t req[7];
    uint8_t raw_bytes[256];
    att_pdu_response pdu;

    characteristic_information.data.find_information_resp.count = 0;

    int current_service = 0;
    int k_characteristics = 1;

    for (int i = 0; i < characteristics.data.read_by_type_resp.count; i++)
    {

        if (k_characteristics >
            services.data.group_type_resp.services[current_service].n_characteristics){
            current_service++;
            k_characteristics = 1;
        }

        uint16_t start_handle = characteristics.data.read_by_type_resp.characteristics[i].value_handle + 1;
        uint16_t end_handle;

        if (k_characteristics == services.data.group_type_resp.services[current_service].n_characteristics){
            end_handle = services.data.group_type_resp.services[current_service].end_handle;
        }
        else        {
            end_handle = characteristics.data.read_by_type_resp.characteristics[i + 1].decl_handle - 1;
        }

        if (start_handle > end_handle){
            k_characteristics++;
            continue;
        }

        while (start_handle <= end_handle){

            att_build_find_information_req(req, start_handle, end_handle);

            send_att_pdu(att_sock, sizeof(req), req);

            int n = read_att_pdu_response(att_sock, sizeof(raw_bytes), raw_bytes);

            if (n < 0){
                printf("Error: read ATT PDU response\n");
                return -1;
            }

            decode_pdu_msg(raw_bytes, n, &pdu);

            int err = parse_att_response(&pdu, &characteristic_information);

            if (err < 0){
                printf("Error: parse ATT response\n");
                return -1;
            }

            if (err == 1)
                break;

            uint8_t count = characteristic_information.data.find_information_resp.count;

            if (count == 0)
                break;

            start_handle = characteristic_information.data.find_information_resp.descriptor_info[count - 1].handle + 1;
        }

        k_characteristics++;
    }

    return 0;
}

int discover_all_services(int att_sock){
    uint16_t start_handle = 0x0001; //All possible handles
    uint16_t end_handle = 0xFFFF;
    uint8_t req[7];
    uint8_t raw_bytes[256];
    att_pdu_response pdu;
    att_parsed_response resp;

    services.data.group_type_resp.count = 0;

    while (1) {
        att_build_read_by_group_type_req(req, start_handle, end_handle);
        send_att_pdu(att_sock, sizeof(req), req); //Build request

        int n = read_att_pdu_response(att_sock, sizeof(raw_bytes), raw_bytes);
        if (n < 0) {
            printf("Error: read pdu response\n");
            return -1;
        }

        decode_pdu_msg(raw_bytes, n, &pdu); 


        int err = parse_att_response(&pdu, &resp); //Parse response

        if (err == 1) {
            if (resp.data.error.error_code == 0x0A) {
                printf("Discovery complete: no more services\n");
                break;
            }
            printf("Unexpected error 0x%02x during discovery\n", resp.data.error.error_code);
            continue;
        }
        else if(err == -1){
            printf("Error: parse att response\n");
            return -1;
        }

        int found = resp.data.group_type_resp.count;
        uint16_t last_end_handle = 0;

        for (int i = 0; i < found; i++) { //Assign discovered services to global struct
            int idx = services.data.group_type_resp.count;
            services.data.group_type_resp.services[idx] = resp.data.group_type_resp.services[i];
            services.data.group_type_resp.count++;
            last_end_handle = resp.data.group_type_resp.services[i].end_handle;
        }

        if (last_end_handle == 0xFFFF) {
            break;
        }

        start_handle = last_end_handle + 1;
    }

    return 0;
}

void get_available_services(void){
    for (int i = 0; i < services.data.group_type_resp.count; i++){
        printf("UUID: ");
        for (int k = 0; k < services.data.group_type_resp.services[i].uuid_len; k++){
            printf("%02X ", services.data.group_type_resp.services[i].uuid[k]);
        }
        printf("\n");
        printf("Start handle: 0x%04X\n", services.data.group_type_resp.services[i].start_handle);
        printf("End handle: 0x%04X\n", services.data.group_type_resp.services[i].end_handle);
    }
}

static const char *uuid16_to_string(uint16_t uuid)
{
    switch (uuid)
    {
        /* ---------- Services ---------- */
        case 0x1800: return "Generic Access";
        case 0x1801: return "Generic Attribute";
        case 0x180A: return "Device Information";
        case 0x180F: return "Battery Service";
        case 0x180D: return "Heart Rate";
        case 0x1812: return "Human Interface Device";
        case 0x181A: return "Environmental Sensing";

        /* ---------- Characteristics ---------- */
        case 0x2A00: return "Device Name";
        case 0x2A01: return "Appearance";
        case 0x2A04: return "Peripheral Preferred Connection Parameters";

        case 0x2A19: return "Battery Level";

        case 0x2A23: return "System ID";
        case 0x2A24: return "Model Number";
        case 0x2A25: return "Serial Number";
        case 0x2A26: return "Firmware Revision";
        case 0x2A27: return "Hardware Revision";
        case 0x2A28: return "Software Revision";
        case 0x2A29: return "Manufacturer Name";

        case 0x2A37: return "Heart Rate Measurement";
        case 0x2A38: return "Body Sensor Location";

        /* ---------- Descriptors ---------- */
        case 0x2900: return "Characteristic Extended Properties";
        case 0x2901: return "Characteristic User Description";
        case 0x2902: return "Client Characteristic Configuration";
        case 0x2903: return "Server Characteristic Configuration";
        case 0x2904: return "Characteristic Presentation Format";
        case 0x2905: return "Characteristic Aggregate Format";
        case 0x2908: return "Valid Range";

        default:
            return "Unknown / Vendor Specific";
    }
}

static void print_properties(uint8_t properties)
{
    printf("    Properties:\n");

    if (properties & 0x01)
        printf("      Broadcast\n");

    if (properties & 0x02)
        printf("      Read\n");

    if (properties & 0x04)
        printf("      Write Without Response\n");

    if (properties & 0x08)
        printf("      Write\n");

    if (properties & 0x10)
        printf("      Notify\n");

    if (properties & 0x20)
        printf("      Indicate\n");

    if (properties & 0x40)
        printf("      Authenticated Signed Write\n");

    if (properties & 0x80)
        printf("      Extended Properties\n");
}

void get_characteristics(void)
{
    printf("\n=========================================================\n");
    printf("                 GATT CHARACTERISTICS\n");
    printf("=========================================================\n");

    for (int i = 0;
         i < characteristics.data.read_by_type_resp.count;
         i++)
    {
        gatt_characteristic_entry *ch =
            &characteristics.data.read_by_type_resp.characteristics[i];

        printf("\nCharacteristic %d\n", i + 1);
        printf("---------------------------------------------\n");

        printf("Declaration Handle : 0x%04X\n",
               ch->decl_handle);

        printf("Value Handle       : 0x%04X\n",
               ch->value_handle);

        printf("Properties Byte    : 0x%02X\n",
               ch->properties);

        print_properties(ch->properties);

        printf("UUID              : ");

        if (ch->uuid_len == 2)
        {
            uint16_t uuid =
                ((uint16_t)ch->uuid[1] << 8) |
                 (uint16_t)ch->uuid[0];

            printf("0x%04X (%s)\n",
                   uuid,
                   uuid16_to_string(uuid));
        }
        else
        {
            printf("128-bit\n");
            printf("    ");

            for (int j = ch->uuid_len - 1; j >= 0; j--)
                printf("%02X", ch->uuid[j]);

            printf("\n");
        }
    }

    printf("\n=========================================================\n");
}
