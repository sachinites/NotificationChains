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

    printf("code = %s, client id = %u , %s\n", 
        notif_chain_get_str_notify_opcode(notif_chain_elem->notif_code),
        notif_chain_elem->client_id,
        (char *)notif_chain_elem->data.app_data_to_notify);
}

static void *
subscriber_thread_fn(void *arg){

   
    uint32_t client_id = (uint32_t)arg;

    if(client_id == 1){
        
        notif_chain_subscribe_by_callback("notif_chain_rt_table", 0, 0, client_id, test_cb);
        pause();
    }
    /* Do registration with Notification Chain.
     * This code acts as a client/subscriber code*/
    rt_entry_keys_t rt_entry_keys;
    memset(&rt_entry_keys, 0, sizeof(rt_entry_keys_t));

    strncpy(rt_entry_keys.dest, "122.1.1.1", 16);
    rt_entry_keys.mask = 32;
    notif_chain_subscribe_by_callback("notif_chain_rt_table", &rt_entry_keys, sizeof(rt_entry_keys_t), client_id, test_cb);

    strncpy(rt_entry_keys.dest, "122.1.1.2", 16);
    rt_entry_keys.mask = 32;
    notif_chain_subscribe_by_callback("notif_chain_rt_table", &rt_entry_keys, sizeof(rt_entry_keys_t), client_id, test_cb);

    strncpy(rt_entry_keys.dest, "122.1.1.3", 16);
    rt_entry_keys.mask = 32;
    notif_chain_subscribe_by_callback("notif_chain_rt_table", &rt_entry_keys, sizeof(rt_entry_keys_t), client_id, test_cb);

    pause();
}

void
create_subscriber_thread(uint32_t client_id){

    pthread_attr_t attr;
    pthread_t subs_thread;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_create(&subs_thread, &attr,
            subscriber_thread_fn,
            (void *)client_id);
}

