/*
 * =====================================================================================
 *
 *       Filename:  notif.c
 *
 *    Description:  This file implements the routines and fns required to implement Notification Chains
 *
 *        Version:  1.0
 *        Created:  08/23/2020 08:19:58 AM
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

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h> // for close
#include <netdb.h>  /*for struct hostent*/
#include "notif.h"

static notif_chain_lookup_by_name_cb 
    notif_chain_lookup_by_name_cb_fn;

void
notif_chain_init(notif_chain_t *notif_chain,
                char *chain_name,
                notif_chain_comp_cb comp_cb,
                app_key_data_print_cb print_cb,
                notif_chain_lookup_by_name_cb 
                    _notif_chain_lookup_by_name_cb_fn){

    struct sockaddr_in notif_chain_addr;
    
    memset(notif_chain, 0, sizeof(notif_chain_t));
    strncpy(notif_chain->name, chain_name, sizeof(notif_chain->name));
    notif_chain->name[sizeof(notif_chain->name) -1] = '\0';
    notif_chain->comp_cb = comp_cb;
    notif_chain->print_cb = print_cb;
    assert(notif_chain_lookup_by_name_cb_fn == NULL);
    notif_chain_lookup_by_name_cb_fn = _notif_chain_lookup_by_name_cb_fn;
}

void
notif_chain_release_communication_channel_resources(
                notif_chain_comm_channel_t *channel){

    notif_ch_type_t notif_ch_type;

    notif_ch_type = channel->notif_ch_type;

    switch(notif_ch_type){

        case NOTIF_C_CALLBACKS:
            return;
        case NOTIF_C_MSG_Q:
            /* Release Msg Q resources here*/
            return;
        case NOTIF_C_AF_UNIX:
            /*Release UNIX Sockets Resources here*/
            return;
        case NOTIF_C_INET_SOCKETS:
            /*Release INET Sockets reources here*/
            return;
        case NOTIF_C_ANY:
        case NOTIF_C_NOT_KNOWN:
        default:    ;
    }
}

void
notif_chain_free_notif_chain_elem(
            notif_chain_elem_t *notif_chain_elem){

        assert(notif_chain_elem->prev == 0 && 
                    notif_chain_elem->next == 0);

        if(notif_chain_elem->data.app_key_data){
            free(notif_chain_elem->data.app_key_data);
        }
        notif_chain_elem->data.app_key_data = 0;
        notif_chain_elem->data.app_key_data_size = 0;
        
        if(notif_chain_elem->data.is_alloc_app_data_to_notify){
            free(notif_chain_elem->data.app_data_to_notify);
        }
        notif_chain_elem->data.app_data_to_notify = 0;
        notif_chain_elem->data.app_data_to_notify_size = 0;
        notif_chain_release_communication_channel_resources(
                &notif_chain_elem->notif_chain_comm_channel);
        free(notif_chain_elem); 
}


void
notif_chain_delete(notif_chain_t *notif_chain){

    notif_chain_elem_t *notif_chain_elem;
    
    ITERTAE_NOTIF_CHAIN_BEGIN(notif_chain, notif_chain_elem){

        notif_chain_elem_remove(notif_chain, notif_chain_elem);
        notif_chain_free_notif_chain_elem(notif_chain_elem);
    } ITERTAE_NOTIF_CHAIN_END(notif_chain, notif_chain_elem);
    notif_chain->head = NULL;
}

bool
notif_chain_register_chain_element(notif_chain_t *notif_chain,
                notif_chain_elem_t *notif_chain_elem){

    notif_chain_elem_t *new_notif_chain_elem = 
        calloc(1,sizeof(notif_chain_elem_t));
   
    if(!new_notif_chain_elem) return false;

    memcpy(new_notif_chain_elem, notif_chain_elem, 
        sizeof(notif_chain_elem_t));

    if(notif_chain_elem->data.app_key_data){
        new_notif_chain_elem->data.app_key_data = 
            calloc(1, notif_chain_elem->data.app_key_data_size);
        memcpy(new_notif_chain_elem->data.app_key_data,
                notif_chain_elem->data.app_key_data, 
                notif_chain_elem->data.app_key_data_size);
        new_notif_chain_elem->data.app_key_data_size = 
            notif_chain_elem->data.app_key_data_size;
    }
    notif_chain_elem_t *head = notif_chain->head;
    notif_chain->head = new_notif_chain_elem;
    new_notif_chain_elem->prev = 0;
    new_notif_chain_elem->next = head;
    if(head)
        head->prev = new_notif_chain_elem;
    return true;
}

bool
notif_chain_communication_channel_match(
                notif_chain_comm_channel_t *channel1,
                notif_chain_comm_channel_t *channel2){

    if(channel1->notif_ch_type != channel2->notif_ch_type)
        return false;

    notif_ch_type_t notif_ch_type = channel1->notif_ch_type;

    switch(notif_ch_type){

        case NOTIF_C_ANY:
            return true;
        case NOTIF_C_CALLBACKS:
            if(NOTIF_CHAIN_ELEM_APP_CB(channel1) != 
                NOTIF_CHAIN_ELEM_APP_CB(channel2))
                return false;
            return true;
        case NOTIF_C_MSG_Q:
            if(strncmp(NOTIF_CHAIN_ELEM_MSGQ_NAME(channel1),
                       NOTIF_CHAIN_ELEM_MSGQ_NAME(channel2),
                       32))
                return false;
            return true;
        case NOTIF_C_AF_UNIX:
            if(strncmp(NOTIF_CHAIN_ELEM_SKT_NAME(channel1),
                       NOTIF_CHAIN_ELEM_SKT_NAME(channel2),
                       32))
                return false;
            return true;
        case NOTIF_C_INET_SOCKETS:
            if((NOTIF_CHAIN_ELEM_IP_ADDR(channel1) == 
                NOTIF_CHAIN_ELEM_IP_ADDR(channel2)) 
                &&
                (NOTIF_CHAIN_ELEM_PORT_NO(channel1) ==
                NOTIF_CHAIN_ELEM_PORT_NO(channel2)))
                return true;
            return false;
        case NOTIF_C_NOT_KNOWN:
            return true;
        default:
            return true;
    }
    return true;
}

bool
notif_chain_deregister_chain_element(notif_chain_t *notif_chain,
                notif_chain_elem_t *notif_chain_elem){

    bool is_removed = false;
    notif_chain_elem_t *notif_chain_elem_curr;
    notif_chain_comm_channel_t *notif_chain_comm_channel;

    notif_chain_comm_channel = &notif_chain_elem->notif_chain_comm_channel;

    ITERTAE_NOTIF_CHAIN_BEGIN(notif_chain, notif_chain_elem_curr){

        if(notif_chain_elem_curr->client_id != 
                notif_chain_elem->client_id){

            continue;
        }

        if(notif_chain->comp_cb &&
                notif_chain->comp_cb(
                    notif_chain_elem_curr->data.app_key_data,
                    notif_chain_elem_curr->data.app_key_data_size,
                    notif_chain_elem->data.app_key_data, 
                    notif_chain_elem->data.app_key_data_size) != 0){

            continue;
        }

        /*Compare connunication channel also*/
        if(notif_chain_comm_channel->notif_ch_type != NOTIF_C_ANY){

            if(notif_chain_comm_channel->notif_ch_type != 
                    notif_chain_elem_curr->notif_chain_comm_channel.notif_ch_type){
                continue;
            }

            /*Now match Notification channel exactness*/
            if(!notif_chain_communication_channel_match(
                        notif_chain_comm_channel,
                        &notif_chain_elem_curr->notif_chain_comm_channel)){

                continue;
            }
        }

        notif_chain_elem_remove(notif_chain, notif_chain_elem_curr);
        notif_chain_free_notif_chain_elem(notif_chain_elem_curr);
        is_removed = true;

    } ITERTAE_NOTIF_CHAIN_END(notif_chain, notif_chain_elem_curr);
    return is_removed;
}

static void
notif_chain_invoke_communication_channel(
        notif_chain_t *notif_chain,
        notif_chain_elem_t *notif_chain_elem){

    notif_chain_comm_channel_t *
        notif_chain_comm_channel = &notif_chain_elem->notif_chain_comm_channel;

    switch(notif_chain_comm_channel->notif_ch_type){

        case NOTIF_C_CALLBACKS:
            assert(NOTIF_CHAIN_ELEM_APP_CB(notif_chain_comm_channel));
            NOTIF_CHAIN_ELEM_APP_CB(notif_chain_comm_channel)(
                notif_chain_elem);
        break;
        case NOTIF_C_MSG_Q:
        break;
        case NOTIF_C_AF_UNIX:
        break;
        case NOTIF_C_INET_SOCKETS:
        break;
        case NOTIF_C_NOT_KNOWN:
        break;
        case NOTIF_C_ANY:
        break;
        default:
            assert(0);
    }
}


bool
notif_chain_invoke(notif_chain_t *notif_chain,
                notif_chain_elem_t *notif_chain_elem){

    notif_chain_elem_t *notif_chain_elem_curr;
    
    ITERTAE_NOTIF_CHAIN_BEGIN(notif_chain, notif_chain_elem_curr){

        if(notif_chain->comp_cb &&
           notif_chain_elem &&
           notif_chain_elem->data.app_key_data &&
           notif_chain_elem->data.app_key_data_size &&
           notif_chain_elem_curr->data.app_key_data &&
           notif_chain_elem_curr->data.app_key_data_size &&
                notif_chain->comp_cb(notif_chain_elem_curr->data.app_key_data,
                    notif_chain_elem_curr->data.app_key_data_size,
                    notif_chain_elem->data.app_key_data,
                    notif_chain_elem->data.app_key_data_size) != 0){

            continue;
        }
        
        notif_chain_elem_curr->notif_code = NOTIF_C_UNKNOWN;

        if(notif_chain_elem){
            notif_chain_elem_curr->notif_code = 
                notif_chain_elem->notif_code;
            notif_chain_elem_curr->data.is_alloc_app_data_to_notify = 
                notif_chain_elem->data.is_alloc_app_data_to_notify;
            notif_chain_elem_curr->data.app_data_to_notify = 
                notif_chain_elem->data.app_data_to_notify;
            notif_chain_elem_curr->data.app_data_to_notify_size =
                notif_chain_elem->data.app_data_to_notify_size;
        }
        notif_chain_invoke_communication_channel(
                notif_chain,
                notif_chain_elem_curr);

    } ITERTAE_NOTIF_CHAIN_END(notif_chain, notif_chain_elem_curr);
}

void
notif_chain_dump(notif_chain_t *notif_chain){

    char buffer[256];

    notif_chain_elem_t *notif_chain_elem_curr;

    printf("notif chain Name : %s\n", notif_chain->name);

    ITERTAE_NOTIF_CHAIN_BEGIN(notif_chain, notif_chain_elem_curr){

        printf("\tprev %p next %p client_id %u app_key_data_ptr %p "
                "app_key_data_size %u app_cb %p\n",
            notif_chain_elem_curr->prev,
            notif_chain_elem_curr->next,
            notif_chain_elem_curr->client_id,
            notif_chain_elem_curr->data.app_key_data,
            notif_chain_elem_curr->data.app_key_data_size,
            NOTIF_CHAIN_ELEM_APP_CB(
                (&notif_chain_elem_curr->notif_chain_comm_channel)));

        if(notif_chain->print_cb){
            notif_chain->print_cb(
                notif_chain_elem_curr->data.app_key_data,
                notif_chain_elem_curr->data.app_key_data_size,
                buffer, sizeof(buffer));
            printf("\tApp Key Data : \n");
            printf("%s\n", buffer);
        }

    } ITERTAE_NOTIF_CHAIN_END(notif_chain, notif_chain_elem_curr);
}

bool
notif_chain_subscribe(char *notif_chain_name,
                      notif_chain_elem_t *notif_chain_elem){

    notif_chain_t *notif_chain;

    notif_ch_type_t notif_ch_type = NOTIF_CHAIN_ELEM_TYPE(notif_chain_elem);
    
    if(!notif_chain_lookup_by_name_cb_fn){
        printf("notif_chain_lookup_by_name_cb CB not registered\n");
        return false;
    }

    notif_chain = (notif_chain_lookup_by_name_cb_fn)(notif_chain_name);
    if(!notif_chain){
        printf("Appln dont have Notif Chain with name %s\n", 
                notif_chain_name);
        return false;
    }

    switch(notif_ch_type){
        
        case NOTIF_C_CALLBACKS:
            notif_chain_register_chain_element(notif_chain, notif_chain_elem);
        break;
        case NOTIF_C_MSG_Q:
        case NOTIF_C_AF_UNIX:
        case NOTIF_C_INET_SOCKETS:
        break;
        case NOTIF_C_ANY:
        case NOTIF_C_NOT_KNOWN:
            return false;
        default:
            return false;
    }
    return true;
}


bool
notif_chain_unsubscribe(char *notif_chain_name,
                      notif_chain_elem_t *notif_chain_elem){

    notif_chain_t *notif_chain;

    notif_ch_type_t notif_ch_type = NOTIF_CHAIN_ELEM_TYPE(notif_chain_elem);
    
    if(!notif_chain_lookup_by_name_cb_fn){
        printf("notif_chain_lookup_by_name_cb CB not registered\n");
        return false;
    }

    notif_chain = (notif_chain_lookup_by_name_cb_fn)(notif_chain_name);

    if(!notif_chain){
        printf("Appln dont have Notif Chain with name %s\n", 
                notif_chain_name);
        return false;
    }

    switch(notif_ch_type){
        
        case NOTIF_C_CALLBACKS:
        case NOTIF_C_MSG_Q:
        case NOTIF_C_AF_UNIX:
        case NOTIF_C_INET_SOCKETS:
            notif_chain_deregister_chain_element(notif_chain, notif_chain_elem);
        break;
        case NOTIF_C_ANY:
        case NOTIF_C_NOT_KNOWN:
            return false;
        default:
            return false;
    }
    return true;
}

void
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

static bool
notif_chain_build_notif_chain_elem_from_subs_msg(
        char *subs_msg, 
        uint32_t subs_msg_size,
        notif_chain_elem_t *notif_chain_elem_template){ // empty template

    memset(notif_chain_elem_template, 0, sizeof(notif_chain_elem_t));
    
    notif_chain_subscriber_msg_format_t *
        notif_chain_subscriber_msg_format = 
        (notif_chain_subscriber_msg_format_t *)subs_msg;

    notif_chain_elem_template->notif_code = 
        notif_chain_subscriber_msg_format->notif_code;

    notif_chain_elem_template->client_id = 
        notif_chain_subscriber_msg_format->client_id;

    memcpy(&notif_chain_elem_template->notif_chain_comm_channel,
           &notif_chain_subscriber_msg_format->notif_chain_comm_channel,
           sizeof(notif_chain_comm_channel_t));

    if(notif_chain_subscriber_msg_format->subs_data &&
        notif_chain_subscriber_msg_format->subs_data_size){
        notif_chain_elem_template->data.app_key_data = 
            calloc(1, notif_chain_subscriber_msg_format->subs_data_size);
        memcpy(notif_chain_elem_template->data.app_key_data,
            notif_chain_subscriber_msg_format->subs_data,
            notif_chain_subscriber_msg_format->subs_data_size);
    }
    return true;
}


bool
notif_chain_process_remote_subscriber_request(
        char *notif_chain_name,
        char *subs_msg, 
        uint32_t subs_msg_size){

    bool res;
    notif_ch_type_t notif_ch_type;
    notif_ch_notify_opcode_t notif_code;
    notif_chain_elem_t notif_chain_elem;

    res = false;
    notif_chain_build_notif_chain_elem_from_subs_msg
                (subs_msg, subs_msg_size, &notif_chain_elem);

    notif_ch_type = NOTIF_CHAIN_ELEM_TYPE((&notif_chain_elem));

    assert(notif_ch_type != NOTIF_C_CALLBACKS);

    notif_code = notif_chain_elem.notif_code;

    if(!notif_chain_lookup_by_name_cb_fn){
        printf("notif_chain_lookup_by_name_cb CB not registered\n");
        return false;
    }

    switch(notif_code){

        case PUB_TO_SUBS_NOTIF_C_CREATE:
        case PUB_TO_SUBS_NOTIF_C_UPDATE:
        case PUB_TO_SUBS_NOTIF_C_DELETE:
            assert(0);
        case SUBS_TO_PUB_NOTIF_C_SUBSCRIBE:
            notif_chain_subscribe(notif_chain_name,
                &notif_chain_elem);
        break;
        case SUBS_TO_PUB_NOTIF_C_UNSUBSCRIBE:
            notif_chain_unsubscribe(notif_chain_name,
                &notif_chain_elem);
        break;
        case SUBS_TO_PUB_NOTIFY_C_NOTIFY_ALL:
        break;
        case SUBS_TO_PUB_NOTIFY_C_CLIENT_UNSUBSCRIBE_ALL:
        break;
        case NOTIF_C_UNKNOWN:
        default:
            return false;
    }
}

/* APIs to be used by client/subscribers*/
bool
notif_chain_subscribe_by_callback(
        char *notif_chain_name,
        void *key,
        uint32_t key_size,
        uint32_t client_id,
        notif_chain_app_cb cb){

    notif_chain_elem_t notif_chain_elem;
    memset(&notif_chain_elem, 0, sizeof(notif_chain_elem_t));
    notif_chain_elem.client_id = client_id;
    notif_chain_elem.notif_code = SUBS_TO_PUB_NOTIF_C_SUBSCRIBE;
    notif_chain_elem.data.app_key_data = key;
    notif_chain_elem.data.app_key_data_size = key_size;
    notif_chain_comm_channel_t *
        notif_chain_comm_channel = &notif_chain_elem.notif_chain_comm_channel;
    notif_chain_comm_channel->notif_ch_type = NOTIF_C_CALLBACKS;
    NOTIF_CHAIN_ELEM_APP_CB(notif_chain_comm_channel) = cb;
    return notif_chain_subscribe(notif_chain_name, &notif_chain_elem);
}

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
        uint16_t publisher_port_no){

    uint32_t msg_size = NOTIF_NAME_SIZE +
                        sizeof(notif_chain_subscriber_msg_format_t) +
                        key_size;

    char *msg = calloc(1, msg_size);

    strncpy(msg, notif_chain_name, NOTIF_NAME_SIZE);

    notif_chain_subscriber_msg_format_t *
        notif_chain_subscriber_msg_format = 
        (notif_chain_subscriber_msg_format_t *)(msg + NOTIF_NAME_SIZE);

    notif_chain_subscriber_msg_format->client_id = client_id;

    notif_chain_subscriber_msg_format->notif_code =
        SUBS_TO_PUB_NOTIF_C_SUBSCRIBE;

    notif_chain_comm_channel_t *notif_chain_comm_channel = 
        &notif_chain_subscriber_msg_format->notif_chain_comm_channel;

    notif_chain_comm_channel->notif_ch_type = NOTIF_C_INET_SOCKETS;
    
    strncpy(NOTIF_CHAIN_ELEM_IP_ADDR(notif_chain_comm_channel),
            subs_addr, 16);
    NOTIF_CHAIN_ELEM_PORT_NO(notif_chain_comm_channel) = subs_port_no;
    NOTIF_CHAIN_ELEM_PROTO(notif_chain_comm_channel) = protocol_no;

    int rc = notif_chain_send_msg_to_publisher(publisher_addr, 
                                      publisher_port_no,
                                      msg, msg_size);
    free(msg);
    return true;
}

bool
notif_chain_subscribe_by_unix_skt(
        char *notif_chain_name,
        void *key,
        uint32_t key_size,
        uint32_t client_id,
        char *subs_unix_skt_name,
        char *publisher_addr,
        uint16_t publisher_port_no){

    return true;
}

bool
notif_chain_subscribe_msgq(
        char *notif_chain_name,
        void *key,
        uint32_t key_size,
        uint32_t client_id,
        char *subs_msgq_name,
        char *publisher_addr,
        uint16_t publisher_port_no){

    return true;
}

static int
notif_chain_send_udp_msg(char *dest_ip_addr,
                         uint16_t dest_port_no,
                         char *msg,
                         uint32_t msg_size){

    struct sockaddr_in dest;

    dest.sin_family = AF_INET;
    dest.sin_port = htons(dest_port_no);
    struct hostent *host = (struct hostent *)gethostbyname(dest_ip_addr);
    dest.sin_addr = *((struct in_addr *)host->h_addr);
    int addr_len = sizeof(struct sockaddr);

    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if(sockfd == -1){
        printf("socket creation failed, errno = %d\n", errno);
        return 0;
    }

    int rc = sendto(sockfd, msg, msg_size,
                    0, (struct sockaddr *)&dest, 
                    sizeof(struct sockaddr));
    close(sockfd);
    return rc; 
}

int
notif_chain_send_msg_to_publisher(char *publisher_addr,
                                  uint16_t publisher_port_no,
                                  char *msg,
                                  uint32_t msg_size){

    return notif_chain_send_udp_msg(publisher_addr, 
                                    publisher_port_no,
                                    msg,
                                    msg_size);
}

int
notif_chain_send_msg_to_subscriber(char *subscriber_addr,
                                   uint16_t subscriber_port_no,
                                   char *msg,
                                   uint32_t msg_size){
    
    return notif_chain_send_udp_msg(subscriber_addr,
                                    subscriber_port_no,
                                    msg,
                                    msg_size);
}

