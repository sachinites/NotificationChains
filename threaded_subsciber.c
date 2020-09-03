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
#include "rt_notif.h"

void
create_subscriber_thread();

static void
test_cb(notif_chain_elem_t *notif_chain_elem){

    printf("code = %s, %s\n", 
        notif_chain_get_str_notify_opcode(notif_chain_elem->notif_code),
        (char *)notif_chain_elem->data.app_data_to_notify);
}

static void *
subscriber_thread_fn(void *arg){

    printf("\n%s() started\n", __FUNCTION__);
    
    /* Do registration with Notification Chain.
     * This code acts as a client/subscriber code*/
    rt_threaded_client_subscribe_for_ip("notif_chain_rt_table", "122.1.1.1", 32, test_cb);
    rt_threaded_client_subscribe_for_ip("notif_chain_rt_table", "122.1.1.2", 32, test_cb);
    rt_threaded_client_subscribe_for_ip("notif_chain_rt_table", "122.1.1.3", 32, test_cb);
    rt_threaded_client_subscribe_for_ip("notif_chain_rt_table", "122.1.1.4", 32, test_cb);
    rt_threaded_client_subscribe_for_ip("notif_chain_rt_table", "122.1.1.5", 32, test_cb);
    rt_threaded_client_subscribe_for_ip("notif_chain_rt_table", "122.1.1.6", 32, test_cb);

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

