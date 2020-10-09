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

static void
process_remote_msgs(char *recv_msg_buffer, 
					uint32_t recv_msg_buffer_size,
					char *sender_ip_addr,
        			uint32_t sender_port_number,
        			uint32_t udp_sock_fd){

	printf("%s() called\n", __FUNCTION__);
}

pthread_t *listener_thread ;

void 
main_menu(){

   	rt_entry_keys_t rt_entry_keys;
	
	int udp_sock_fd = -1;

    strncpy(rt_entry_keys.dest, "122.1.1.6", 16);
    rt_entry_keys.mask = 32;

	/* Register for some sample entries */
	udp_sock_fd = notif_chain_subscribe_by_inet_skt(
		"notif_chain_rt_table",
		&rt_entry_keys,
		sizeof(rt_entry_keys_t),
		getpid(),
		"127.0.0.1",
		2001,
		IPPROTO_UDP,
		"127.0.0.1",
		2000,
		SUBS_TO_PUB_NOTIF_C_SUBSCRIBE, udp_sock_fd);

	strncpy(rt_entry_keys.dest, "122.1.1.7", 16);
    rt_entry_keys.mask = 32;
	
	udp_sock_fd = notif_chain_subscribe_by_inet_skt(
		"notif_chain_rt_table",
		&rt_entry_keys,
		sizeof(rt_entry_keys_t),
		getpid(),
		"127.0.0.1",
		2001,
		IPPROTO_UDP,
		"127.0.0.1",
		2000,
		SUBS_TO_PUB_NOTIF_C_SUBSCRIBE,
		udp_sock_fd);
#if 0
	sleep(5);
    strncpy(rt_entry_keys.dest, "122.1.1.3", 16);
    rt_entry_keys.mask = 32;

	/* Register for some sample entries */
	udp_sock_fd = notif_chain_subscribe_by_inet_skt(
		"notif_chain_rt_table",
		&rt_entry_keys,
		sizeof(rt_entry_keys_t),
		getpid(),
		"127.0.0.1",
		2001,
		IPPROTO_UDP,
		"127.0.0.1",
		2000,
		SUBS_TO_PUB_NOTIF_C_UNSUBSCRIBE,
		udp_sock_fd);
	
	strncpy(rt_entry_keys.dest, "122.1.1.4", 16);
    rt_entry_keys.mask = 32;
	
	udp_sock_fd = notif_chain_subscribe_by_inet_skt(
		"notif_chain_rt_table",
		&rt_entry_keys,
		sizeof(rt_entry_keys_t),
		getpid(),
		"127.0.0.1",
		2001,
		IPPROTO_UDP,
		"127.0.0.1",
		2000,
		SUBS_TO_PUB_NOTIF_C_UNSUBSCRIBE,
		udp_sock_fd);	
	
	sleep(5);
#endif
    /*Start the pkt receiever thread*/
	listener_thread = 
		network_start_listening_on_comm_fd(udp_sock_fd,
		process_remote_msgs);
}

int
main(int argc, char **argv){


    main_menu();
	pause();
	pthread_cancel(*listener_thread );
	pthread_join(*listener_thread, 0);
	free(listener_thread);
	listener_thread = NULL;
    return 0;
}

