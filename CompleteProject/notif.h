/*
 * =====================================================================================
 *
 *       Filename:  notif.h
 *
 *    Description:  This file defines the Structure definition required to implement Notification Chains
 *
 *        Version:  1.0
 *        Created:  08/23/2020 08:05:55 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Er. Abhishek Sagar, Juniper Networks (https://csepracticals.wixsite.com/csepracticals), sachinites@gmail.com
 *        Company:  Juniper Networks
 *
 *        This file is part of the NotificationChains distribution (https://github.com/sachinites) 
 *        Copynext (c) 2019 Abhishek Sagar.
 *        This program is free software: you can redistribute it and/or modify it under the terms of the GNU General 
 *        Public License as published by the Free Software Foundation, version 3.
 *        
 *        This program is distributed in the hope that it will be useful, but
 *        WITHOUT ANY WARRANTY; without even the implied warranty of
 *        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *        General Public License for more details.
 *
 *        visit website : https://csepracticals.wixsite.com/csepracticals for more courses and projects
 *                                  
 * =====================================================================================
 */

#ifndef __NOTIF_H__
#define __NOTIF_H__

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include "gluethread/glthread.h"

typedef struct notif_chain_elem_ notif_chain_elem_t;
typedef struct notif_chain_ notif_chain_t;
typedef int (*notif_chain_comp_cb)(void *data1, uint32_t data_size1,
                                   void *data2, uint32_t data_size2);
typedef void (*notif_chain_app_cb)(notif_chain_elem_t *notif_chain_elem);
typedef char * (*app_key_data_print_cb)(void *data,
                        uint32_t size,
                        char *outbuffer,
                        uint32_t outbuffer_size);

#define NOTIF_NAME_SIZE 	32
#define NOTIF_CHAIN_PROTO	240

typedef enum notif_ch_type_{

    NOTIF_C_ANY,
    NOTIF_C_CALLBACKS,
    NOTIF_C_MSG_Q,
    NOTIF_C_AF_UNIX,
    NOTIF_C_INET_SOCKETS,
    NOTIF_C_NOT_KNOWN
} notif_ch_type_t;

static inline char *
notif_chain_get_str_notif_ch_type(
    notif_ch_type_t notif_ch_type){

    switch(notif_ch_type){

        case NOTIF_C_CALLBACKS:
            return "NOTIF_C_CALLBACKS";
        case NOTIF_C_MSG_Q:
            return "NOTIF_C_MSG_Q";
        case NOTIF_C_AF_UNIX:
            return "NOTIF_C_AF_UNIX";
        case NOTIF_C_INET_SOCKETS:
            return "NOTIF_C_INET_SOCKETS";
        case NOTIF_C_NOT_KNOWN:
            return "NOTIF_C_NOT_KNOWN";
        default:
            assert(0);
    }
}

typedef enum notif_ch_notify_opcode_{

    /* Used by Publisher to tell 
     * subscriber the nature of 
     * Update*/
    PUB_TO_SUBS_NOTIF_C_CREATE,
    PUB_TO_SUBS_NOTIF_C_UPDATE,
    PUB_TO_SUBS_NOTIF_C_DELETE,
    /* Used by Subscriber to tell
     * publisher about the type
     * of subscription*/
    SUBS_TO_PUB_NOTIF_C_SUBSCRIBE,
    SUBS_TO_PUB_NOTIF_C_UNSUBSCRIBE,
    SUBS_TO_PUB_NOTIFY_C_NOTIFY_ALL,
    SUBS_TO_PUB_NOTIFY_C_CLIENT_UNSUBSCRIBE_ALL,
    NOTIF_C_UNKNOWN
} notif_ch_notify_opcode_t;

static inline char *
notif_chain_get_str_notify_opcode(
    notif_ch_notify_opcode_t notif_opcode){

    switch(notif_opcode){

        case PUB_TO_SUBS_NOTIF_C_CREATE:
            return "PUB_TO_SUBS_NOTIF_C_CREATE";
        case PUB_TO_SUBS_NOTIF_C_UPDATE:
            return "PUB_TO_SUBS_NOTIF_C_UPDATE";
        case PUB_TO_SUBS_NOTIF_C_DELETE:
            return "PUB_TO_SUBS_NOTIF_C_DELETE";
        case SUBS_TO_PUB_NOTIF_C_SUBSCRIBE:
            return "SUBS_TO_PUB_NOTIF_C_SUBSCRIBE";
        case SUBS_TO_PUB_NOTIF_C_UNSUBSCRIBE:
            return "SUBS_TO_PUB_NOTIF_C_UNSUBSCRIBE";
        case SUBS_TO_PUB_NOTIFY_C_NOTIFY_ALL:
            return "SUBS_TO_PUB_NOTIFY_C_NOTIFY_ALL";
        case SUBS_TO_PUB_NOTIFY_C_CLIENT_UNSUBSCRIBE_ALL:
            return "SUBS_TO_PUB_NOTIFY_C_CLIENT_UNSUBSCRIBE_ALL";
        case NOTIF_C_UNKNOWN:
            return "NOTIF_C_UNKNOWN";
        default:
            assert(0);
    }
}

typedef struct notif_chain_comm_channel_{
    
    notif_ch_type_t notif_ch_type;
    union {
        /*Via Callbacks*/
        notif_chain_app_cb app_cb;
        /*Via MsgQ*/
        struct {
            char msgQ_name[NOTIF_NAME_SIZE];
        } mq;

        /*Via UNIX Sockets*/
        struct {
            char unix_skt_name[NOTIF_NAME_SIZE];
        } unix_skt;
        /*Via INET_SOCKETS*/
        struct {
            uint32_t ip_addr;
            uint16_t port_no;
            uint8_t protocol_no; /*UDP or TCP*/
			uint32_t skf_fd;	 /*Skt FD created by the Publisher*/
        } inet_skt_info;
    }u;
	uint32_t ref_count;			/* No of entries using this comm channel */
	glthread_t glue;
} notif_chain_comm_channel_t;
GLTHREAD_TO_STRUCT(glthread_glue_to_notif_chain_comm_channel,
					notif_chain_comm_channel_t, glue);

#define NOTIF_CHAIN_ELEM_APP_CB(notif_chain_comm_channel_ptr)           \
    (notif_chain_comm_channel_ptr->u.app_cb)
#define NOTIF_CHAIN_ELEM_MSGQ_NAME(notif_chain_comm_channel_ptr)        \
    (notif_chain_comm_channel_ptr->u.mq.msgQ_name)
#define NOTIF_CHAIN_ELEM_SKT_NAME(notif_chain_comm_channel_ptr)         \
    (notif_chain_comm_channel_ptr->u.unix_skt.unix_skt_name)
#define NOTIF_CHAIN_ELEM_IP_ADDR(notif_chain_comm_channel_ptr)          \
    (notif_chain_comm_channel_ptr->u.inet_skt_info.ip_addr)
#define NOTIF_CHAIN_ELEM_PORT_NO(notif_chain_comm_channel_ptr)          \
    (notif_chain_comm_channel_ptr->u.inet_skt_info.port_no)
#define NOTIF_CHAIN_ELEM_PROTO(notif_chain_comm_channel_ptr)            \
    (notif_chain_comm_channel_ptr->u.inet_skt_info.protocol_no)
#define NOTIF_CHAIN_ELEM_SKT_FD(notif_chain_comm_channel_ptr)			\
	(notif_chain_comm_channel_ptr->u.inet_skt_info.skf_fd)

struct notif_chain_elem_{

	notif_chain_t *notif_chain;
   
    uint32_t client_id;
    notif_ch_notify_opcode_t notif_code;
    
    struct {
        /* Key data to decide which 
         * subscriber to notify
         * */
        void *app_key_data;
        uint32_t app_key_data_size;

        /* Once the subscriber is chosen, 
         * NCM must send notif to subscriber 
         * with this data*/ 
        bool is_alloc_app_data_to_notify;
        void *app_data_to_notify;
        uint32_t app_data_to_notify_size;
    } data;

    notif_chain_comm_channel_t 
        *notif_chain_comm_channel;
	glthread_t glue;
};
GLTHREAD_TO_STRUCT(glthread_glue_to_notif_chain_elem,
					notif_chain_elem_t, glue);

#define NOTIF_CHAIN_COMM_CH_TYPE(notif_chain_elem_ptr)                     \
    (notif_chain_elem_ptr->notif_chain_comm_channel->notif_ch_type)

struct notif_chain_{

    char name[NOTIF_NAME_SIZE];
    /* Comparison fn to compare app_key_data 
     * present in notif_chain_elem_t objects
     * present in chain. This can be NULL if
     * comparison is not required
     * */
    notif_chain_comp_cb comp_cb;
    app_key_data_print_cb print_cb;

    /* Head of the linked list containing
     * notif_chain_elem_t
     * */
    glthread_t notif_chain_elem_head;
	glthread_t glue;
};
GLTHREAD_TO_STRUCT(glthread_glue_to_notif_chain,
					notif_chain_t, glue);

typedef struct comm_channel_per_client_db_{

	uint32_t client_id;
	glthread_t comm_channel_head;
	glthread_t glue;	
} comm_channel_per_client_db_t;
GLTHREAD_TO_STRUCT(glthread_glue_to_comm_channel_per_client_db,
                    comm_channel_per_client_db_t, glue);

typedef struct notif_chain_db_ {

	glthread_t notif_chain_head;
	glthread_t comm_channel_per_client_db_head;
} notif_chain_db_t;

notif_chain_comm_channel_t *
notif_chain_lookup_matching_comm_channel_per_client(
		notif_chain_db_t *notif_chain_db,
		uint32_t client_id,
		notif_chain_comm_channel_t *notif_chain_comm_channel_template);

notif_chain_comm_channel_t *
notif_chain_record_comm_channel_per_client(
		notif_chain_db_t *notif_chain_db,
		uint32_t client_id,
        notif_chain_comm_channel_t *notif_chain_comm_channel_template);	

bool
notif_chain_communication_channel_match(
                notif_chain_comm_channel_t *channel1,
                notif_chain_comm_channel_t *channel2);

void
notif_chain_release_communication_channel_resources(
                notif_chain_comm_channel_t *channel);

void
notif_chain_free_notif_chain_elem(
                notif_chain_elem_t *notif_chain_elem);
void
notif_chain_free_notif_chain_elem_internals(
                notif_chain_elem_t *notif_chain_elem);

void
notif_chain_init(notif_chain_t *notif_chain,
                 char *chain_name,
                 notif_chain_comp_cb comp_cb,
                 app_key_data_print_cb print_cb);

bool
notif_chain_is_matching_notif_chain_element(
                notif_chain_elem_t *notif_chain_elem,
                uint32_t client_id,
                void *app_key_data,
                uint32_t app_key_data_size);

notif_chain_elem_t *
notif_chain_lookup_notif_chain_element(
				notif_chain_t *notif_chain,
				uint32_t client_id,
				void *app_key_data,
				uint32_t app_key_data_size);

void
notif_chain_delete(notif_chain_t *notif_chain);

bool
notif_chain_register_chain_element(notif_chain_t *notif_chain,
                notif_chain_elem_t *notif_chain_elem);

bool
notif_chain_deregister_chain_element(notif_chain_t *notif_chain,
                notif_chain_elem_t *notif_chain_elem);

void
notif_chain_invoke(notif_chain_t *notif_chain,
                notif_chain_elem_t *notif_chain_elem);

void
notif_chain_dump(notif_chain_t *notif_chain);

bool
notif_chain_subscribe(char *notif_name, 
                      notif_chain_elem_t *notif_chain_elem);

bool
notif_chain_unsubscribe(char *notif_name, 
                      notif_chain_elem_t *notif_chain_elem);

bool
notif_chain_process_remote_subscriber_request(
                     char *subs_tlv_buffer,
                     uint32_t subs_tlv_buffer_size);

/* Subscription APIs to be used by Subscribers*/
int
notif_chain_send_msg_to_publisher(char *publisher_addr,
                                  uint16_t publisher_port_no,
                                  char *msg, uint32_t msg_size);

bool
notif_chain_subscribe_by_callback(
        char *notif_chain_name,
        void *key,
        uint32_t key_size,
        uint32_t client_id,
        notif_chain_app_cb cb);

bool
notif_chain_subscribe_by_inet_skt(
        char *notif_chain_name,
        void *key,
        uint32_t key_size,
        uint32_t client_id,
        char *subs_addr,
        uint16_t subs_port_no,
        uint16_t protocol_no,
        char *publisher_addr,
        uint16_t publisher_port_no);

bool
notif_chain_subscribe_by_unix_skt(
        char *notif_chain_name,
        void *key,
        uint32_t key_size,
        uint32_t client_id,
        char *subs_unix_skt_name,
        char *publisher_addr,
        uint16_t publisher_port_no);

bool
notif_chain_subscribe_msgq(
        char *notif_chain_name,
        void *key,
        uint32_t key_size,
        uint32_t client_id,
        char *subs_msgq_name,
        char *publisher_addr,
        uint16_t publisher_port_no);

int
notif_chain_send_msg_to_publisher(char *publisher_addr,
                                  uint16_t publisher_port_no,
                                  char *msg,
                                  uint32_t msg_size);
int
notif_chain_send_msg_to_subscriber(char *subscriber_addr,
                                   uint16_t subscriber_port_no,
                                   char *msg,
                                   uint32_t msg_size);

/* TLV Management */
#define NOTIF_C_NOTIF_CHAIN_NAME_TLV    (1)  /* Value len : NOTIF_NAME_SIZE */
#define NOTIF_C_CLIENT_ID_TLV           (2)  /* Value len : 4 */
#define NOTIF_C_COMM_CHANNEL_TYPE_TLV   (3)  /* Value len : 1 */
#define NOTIF_C_COMM_CHANNEL_NAME_TLV   (4)  /* Value len : NOTIF_NAME_SIZE */
#define NOTIF_C_IP_ADDR_TLV             (5)  /* Value len : 4 , integer format */
#define NOTIF_C_PORT_NO_TLV             (6)  /* Value len : 2 , integer format */
#define NOTIF_C_NOTIF_CODE_TLV          (7)  /* Value len : 1 */
#define NOTIF_C_PROTOCOL_NO_TLV         (8)  /* Value len : 1 */
#define NOTIF_C_APP_KEY_DATA_TLV        (9)  /* Variable Size */
#define NOTIF_C_APP_DATA_TO_NOTIFY_TLV  (10) /* Variable Size */
#define NOTIF_C_MAX_TLV                 (11)

#define NOTIF_C_NOTIF_CHAIN_NAME_VALUE_LEN  (FIELD_SIZE(notif_chain_t, name))
#define NOTIF_C_CLIENT_ID_VALUE_LEN         (FIELD_SIZE(notif_chain_elem_t, client_id))
#define NOTIF_C_COMM_CHANNEL_TYPE_VALUE_LEN (FIELD_SIZE(notif_chain_comm_channel_t, notif_ch_type)) 
#define NOTIF_C_COMM_CHANNEL_MSGQ_NAME_VALUE_LEN     \
    (FIELD_SIZE(notif_chain_comm_channel_t, u.mq.msgQ_name))
#define NOTIF_C_COMM_CHANNEL_UNIX_SKT_NAME_VALUE_LEN \
    (FIELD_SIZE(notif_chain_comm_channel_t, u.unix_skt.unix_skt_name))
#define NOTIF_C_IP_ADDR_VALUE_LEN           (FIELD_SIZE(notif_chain_comm_channel_t, u.inet_skt_info.ip_addr))
#define NOTIF_C_PORT_NO_VALUE_LEN           (FIELD_SIZE(notif_chain_comm_channel_t, u.inet_skt_info.port_no))
#define NOTIF_C_NOTIF_CODE_VALUE_LEN        (FIELD_SIZE(notif_chain_elem_t, notif_code)) 
#define NOTIF_C_PROTOCOL_NO_VALUE_LEN       (FIELD_SIZE(notif_chain_comm_channel_t, u.inet_skt_info.protocol_no))

uint32_t
notif_chain_compute_required_tlv_buffer_size_for_notif_chain_elem_encoding(
    notif_chain_elem_t *notif_chain_elem);

uint32_t
notif_chain_serialize_notif_chain_elem(char *notif_chain_name,
                                    notif_chain_elem_t *notif_chain_elem,
                                    char *output_buffer_provided,
                                    uint32_t output_buffer_provided_size,
                                    char **output_buffer_computed);

notif_chain_elem_t *
notif_chain_deserialize_notif_chain_elem(
                char *tlv_buffer,
                uint32_t tlv_buff_size,
                char *notif_chain_name /*o/p*/);

#endif /* __NOTIF_H__ */
