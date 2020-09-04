/*
 * =====================================================================================
 *
 *       Filename:  utils.c
 *
 *    Description: This file contains general utility routines 
 *
 *        Version:  1.0
 *        Created:  Saturday 21 September 2019 06:03:54  IST
 *       Revision:  1.0
 *       Compiler:  gcc
 *
 *         Author:  Er. Abhishek Sagar, Networking Developer (AS), sachinites@gmail.com
 *        Company:  Brocade Communications(Jul 2012- Mar 2016), Current : Juniper Networks(Apr 2017 - Present)
 *        
 *        This file is part of the NetworkGraph distribution (https://github.com/sachinites).
 *        Copyright (c) 2017 Abhishek Sagar.
 *        This program is free software: you can redistribute it and/or modify
 *        it under the terms of the GNU General Public License as published by  
 *        the Free Software Foundation, version 3.
 *
 *        This program is distributed in the hope that it will be useful, but 
 *        WITHOUT ANY WARRANTY; without even the implied warranty of 
 *        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 *        General Public License for more details.
 *
 *        You should have received a copy of the GNU General Public License 
 *        along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * =====================================================================================
 */

#include <sys/socket.h>
#include <arpa/inet.h> /*for inet_ntop & inet_pton*/
#include <stdint.h>
#include <memory.h>
#include "utils.h"

char *
tcp_ip_covert_ip_n_to_p(uint32_t ip_addr, 
                    char *output_buffer){

    char *out = NULL;
    static char str_ip[16];
    out = !output_buffer ? str_ip : output_buffer;
    memset(out, 0, 16);
    ip_addr = htonl(ip_addr);
    inet_ntop(AF_INET, &ip_addr, out, 16);
    out[15] = '\0';
    return out;
}

uint32_t
tcp_ip_covert_ip_p_to_n(char *ip_addr){

    uint32_t binary_prefix = 0;
    inet_pton(AF_INET, ip_addr, &binary_prefix);
    binary_prefix = htonl(binary_prefix);
    return binary_prefix;
}

char *
tlv_buffer_insert_tlv(char *buff, uint8_t tlv_no,
                     uint8_t data_len, char *data){

    *buff = tlv_no;
    *(buff+ sizeof(tlv_no)) = data_len;
    memcpy(buff + TLV_OVERHEAD_SIZE, data, data_len);
    return buff + TLV_OVERHEAD_SIZE + data_len;
}

char *
tlv_buffer_get_particular_tlv(char *tlv_buff, /*Input TLV Buffer*/
                      uint32_t tlv_buff_size, /*Input TLV Buffer Total Size*/
                      uint8_t tlv_no,         /*Input TLV Number*/
                      uint8_t *tlv_data_len){ /*Output TLV Data len*/

    uint8_t tlv_type, tlv_len, *tlv_value = NULL;
    
    ITERATE_TLV_BEGIN(tlv_buff, tlv_type, tlv_len, tlv_value, tlv_buff_size){
        
        if(tlv_type != tlv_no) continue;
        *tlv_data_len = tlv_len;
        return tlv_value;
    }ITERATE_TLV_END(tlv_buff, tlv_type, tlv_len, tlv_value, tlv_buff_size); 

    *tlv_data_len = 0;

    return NULL;
}
