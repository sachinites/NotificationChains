/*
 * =====================================================================================
 *
 *       Filename:  test.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/29/2020 02:38:48 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Er. Abhishek Sagar, Juniper Networks (https://csepracticals.wixsite.com/csepracticals), sachinites@gmail.com
 *        Company:  Juniper Networks
 *
 *        This file is part of the NotificationChains distribution (https://github.com/sachinites) 
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "notif.h"
#include "rt.h"
#include "network_utils.h"

void 
main_menu(){

   	rt_entry_keys_t rt_entry_keys;

while(1) {

    strncpy(rt_entry_keys.dest, "122.1.1.3", 16);
    rt_entry_keys.mask = 32;

	/* Register for some sample entries */
	notif_chain_subscribe_by_inet_skt(
		"notif_chain_rt_table",
		&rt_entry_keys,
		sizeof(rt_entry_keys_t),
		getpid(),
		"127.0.0.1",
		2001,
		NOTIF_CHAIN_PROTO,
		"127.0.0.1",
		2000,
		SUBS_TO_PUB_NOTIF_C_SUBSCRIBE);

    strncpy(rt_entry_keys.dest, "122.1.1.4", 16);
    rt_entry_keys.mask = 32;
	
	notif_chain_subscribe_by_inet_skt(
		"notif_chain_rt_table",
		&rt_entry_keys,
		sizeof(rt_entry_keys_t),
		getpid(),
		"127.0.0.1",
		2001,
		NOTIF_CHAIN_PROTO,
		"127.0.0.1",
		2000,
		SUBS_TO_PUB_NOTIF_C_SUBSCRIBE);

	sleep(5);

    strncpy(rt_entry_keys.dest, "122.1.1.3", 16);
    rt_entry_keys.mask = 32;

	/* Register for some sample entries */
	notif_chain_subscribe_by_inet_skt(
		"notif_chain_rt_table",
		&rt_entry_keys,
		sizeof(rt_entry_keys_t),
		getpid(),
		"127.0.0.1",
		2001,
		NOTIF_CHAIN_PROTO,
		"127.0.0.1",
		2000,
		SUBS_TO_PUB_NOTIF_C_UNSUBSCRIBE);

    strncpy(rt_entry_keys.dest, "122.1.1.4", 16);
    rt_entry_keys.mask = 32;
	
	notif_chain_subscribe_by_inet_skt(
		"notif_chain_rt_table",
		&rt_entry_keys,
		sizeof(rt_entry_keys_t),
		getpid(),
		"127.0.0.1",
		2001,
		NOTIF_CHAIN_PROTO,
		"127.0.0.1",
		2000,
		SUBS_TO_PUB_NOTIF_C_UNSUBSCRIBE);	

		sleep(5);
	}
}

static void
process_remote_msgs(char *recv_msg_buffer, 
					uint32_t recv_msg_buffer_size,
					char *sender_ip_addr,
        			uint32_t sender_port_number,
        			uint32_t sender_skt_fd,
        			int fd_set_arr[]){


}

int
main(int argc, char **argv){


    /*Start the pkt receiever thread*/
	network_start_udp_pkt_receiver_thread(
		"127.0.0.1",
		2001,
		process_remote_msgs);
    main_menu();
    return 0;
}

