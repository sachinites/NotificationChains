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

typedef struct notif_chain_elem_ notif_chain_elem_t;
typedef struct notif_chain_ notif_chain_t;
typedef int (*notif_chain_comp_fn)(void *data1, uint32_t data_size1,
                                   void *data2, uint32_t data_size2);
typedef void (*notif_chain_app_cb)(void *data, uint32_t data_size);

typedef enum notif_ch_type_{

    NOTIF_C_CALLBACKS,
    NOTIF_C_MSG_Q,
    NOTIF_C_AF_UNIX,
    NOTIF_C_INET_SOCKETS,
    NOTIF_C_NOT_KNOWN
} notif_ch_type_t;

typedef struct notif_chain_comm_channel_{
    
    notif_ch_type_t notif_ch_type;
    union {
        /*Via Callbacks*/
        notif_chain_app_cb app_cb;
        /*Via MsgQ*/
        char msgQ_name[32];
        /*Via UNIX Sockets*/
        char unix_skt_name[32];
        /*Via INET_SOCKETS*/
        struct {

            uint32_t ip_addr;
            uint8_t port_no;
            uint8_t protocol_no; /*UDP or TCP or simply IP*/
        } inet_skt_info;
    }u;
} notif_chain_comm_channel_t;

struct notif_chain_elem_{

    notif_chain_elem_t *prev;
    notif_chain_elem_t *next;

    union {

        /* Key data to decide which 
         * subscriber to notify
         * */
        struct {
            void *app_key_data;
            uint32_t app_key_data_size;
        } app_key_data_info;

        /* Once the subscriber is chosen, 
         * NCM must send notif to subscriber 
         * with this data*/
        struct {
            void *app_data_to_notify;
            uint32_t app_data_to_notify_size;
        } app_data_to_notify_info;

    } data;

    notif_chain_comm_channel_t 
        notif_chain_comm_channel;
};

struct notif_chain_{

    char name[32];
    /* Comparison fn to compare app_key_data 
     * present in notif_chain_elem_t objects
     * present in chain. This can be NULL if
     * comparison is not required
     * */
    notif_chain_comp_fn comp_fn;

    /* Head of the linked list containing
     * notif_chain_elem_t
     * */
    notif_chain_elem_t *head;
};

void
notif_chain_init(notif_chain_t *notif_chain,
                 char *chain_name,
                 notif_chain_comp_fn comp_fn);

void
notif_chain_delete(notif_chain_t *notif_chain);

bool
notif_chain_register_chain_element(notif_chain_t *notif_chain,
                notif_chain_elem_t *notif_chain_elem);

bool
notif_chain_deregister_chain_element(notif_chain_t *notif_chain,
                notif_chain_elem_t *notif_chain_elem);

bool
notif_chain_invoke(notif_chain_t *notif_chain,
                notif_chain_elem_t *notif_chain_elem);

static inline void
notif_chain_elem_remove(notif_chain_t *notif_chain,
                notif_chain_elem_t *notif_chain_elem){

    if(!notif_chain_elem->prev){
        if(notif_chain_elem->next){
            notif_chain_elem->next->prev = NULL;
            notif_chain->head = notif_chain_elem->next;
            notif_chain_elem->next = 0;
            return;
        }
        return;
    }
    if(!notif_chain_elem->next){
        notif_chain_elem->prev->next = NULL;
        notif_chain_elem->prev = NULL;
        return;
    }

    notif_chain_elem->prev->next = notif_chain_elem->next;
    notif_chain_elem->next->prev = notif_chain_elem->prev;
    notif_chain_elem->prev = 0;
    notif_chain_elem->next = 0;
}

#define ITERTAE_NOTIF_CHAIN_BEGIN(notif_chain_ptr, notif_chain_elem_ptr)  \
{\
    notif_chain_elem_t *_next_notif_chain_elem;\
    for(notif_chain_elem_ptr = notif_chain_ptr->head; \
        notif_chain_elem_ptr; \
        notif_chain_elem_ptr = _next_notif_chain_elem) { \
        _next_notif_chain_elem = notif_chain_elem_ptr->next;

#define ITERTAE_NOTIF_CHAIN_END(notif_chain_ptr, notif_chain_elem_ptr)  }}

void
notif_chain_dump(notif_chain_t *notif_chain);

#endif /* __NOTIF_H__ */
