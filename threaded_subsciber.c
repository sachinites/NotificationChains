/*
 * =====================================================================================
 *
 *       Filename:  threaded_subsciber.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/31/2020 02:08:36 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Er. Abhishek Sagar, Juniper Networks (https://csepracticals.wixsite.com/csepracticals), sachinites@gmail.com
 *        Company:  Juniper Networks
 *
 *        This file is part of the XXX distribution (https://github.com/sachinites) 
 *        Copyright (c) 2019 Abhishek Sagar.
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

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/types.h> /*for pid_t*/
#include <unistd.h>    /*for getpid()*/
#include <pthread.h>
#include <stdbool.h>
#include "notif.h"
#include "rt.h"

void
create_subscriber_thread();

static void
test_cb(notif_chain_elem_t *notif_chain_elem){

    printf("code = %s, %s\n", 
        notif_chain_get_str_notify_opcode(notif_chain_elem->notif_code),
        (char *)notif_chain_elem->data.app_data_to_notify);
}

static bool
threaded_client_subscribe_for_ip(
    char *notif_chain_name, 
    char *ip,
    char mask){

    notif_chain_elem_t notif_chain_elem;
    memset(&notif_chain_elem, 0, sizeof(notif_chain_elem_t));
    notif_chain_elem.client_pid = getpid();
    notif_chain_elem.notif_code = SUBS_TO_PUB_NOTIF_C_SUBSCRIBE;
    rt_entry_keys_t rt_entry_keys;
    strncpy(rt_entry_keys.dest, ip, 16);
    rt_entry_keys.mask = mask;
    notif_chain_elem.data.app_key_data = (void *)&rt_entry_keys;
    notif_chain_elem.data.app_key_data_size = sizeof(rt_entry_keys_t);
    notif_chain_comm_channel_t *
        notif_chain_comm_channel = &notif_chain_elem.notif_chain_comm_channel;
    notif_chain_comm_channel->notif_ch_type = NOTIF_C_CALLBACKS;
    NOTIF_CHAIN_ELEM_APP_CB(notif_chain_comm_channel) = test_cb;
    return notif_chain_subscribe(notif_chain_name, &notif_chain_elem);
}

static void *
subscriber_thread_fn(void *arg){

    printf("\n%s() started\n", __FUNCTION__);
    
    /* Do registration with Notification Chain.
     * This code acts as a client/subscriber code*/
    threaded_client_subscribe_for_ip("notif_chain_rt_table", "122.1.1.1", 32);
    threaded_client_subscribe_for_ip("notif_chain_rt_table", "122.1.1.2", 32);
    threaded_client_subscribe_for_ip("notif_chain_rt_table", "122.1.1.3", 32);
    threaded_client_subscribe_for_ip("notif_chain_rt_table", "122.1.1.4", 32);
    threaded_client_subscribe_for_ip("notif_chain_rt_table", "122.1.1.5", 32);
    threaded_client_subscribe_for_ip("notif_chain_rt_table", "122.1.1.6", 32);

    pause();
}

void
create_subscriber_thread(){

    pthread_attr_t attr;
    pthread_t subs_thread;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_create(&subs_thread, &attr,
            subscriber_thread_fn,
            NULL);
}

