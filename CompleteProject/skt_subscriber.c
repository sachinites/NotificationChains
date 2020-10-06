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
#include "notif.h"
#include "rt.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void network_start_pkt_receiver_thread(void );

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

int
main(int argc, char **argv){


    /*Start the pkt receiever thread*/
    network_start_pkt_receiver_thread();
    main_menu();
    return 0;
}

#define MAX_PACKET_BUFFER_SIZE 1024

static char recv_buffer[MAX_PACKET_BUFFER_SIZE];

static void
process_remote_msgs(char *recv_msg_buffer, 
					uint32_t recv_msg_buffer_size){


}


static void *
_network_start_pkt_receiver_thread(void *arg){

    int udp_sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP );

    if(udp_sock_fd == -1){
        printf("Socket Creation Failed\n");
        return 0;   
    }

    struct sockaddr_in publisher_addr;
    publisher_addr.sin_family      = AF_INET;
    publisher_addr.sin_port        = 2001;
    publisher_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udp_sock_fd, (struct sockaddr *)&publisher_addr, 
		sizeof(struct sockaddr)) == -1) {
        printf("Error : socket bind failed\n");
        return 0;
    }

    fd_set active_sock_fd_set,
           backup_sock_fd_set;

    FD_ZERO(&active_sock_fd_set);
    FD_ZERO(&backup_sock_fd_set);

    struct sockaddr_in subscriber_addr;
    FD_SET(udp_sock_fd, &backup_sock_fd_set);
    int bytes_recvd = 0,
		addr_len = sizeof(subscriber_addr);
	
    while(1){

        memcpy(&active_sock_fd_set, &backup_sock_fd_set, sizeof(fd_set));
        select(udp_sock_fd + 1, &active_sock_fd_set, NULL, NULL, NULL);
        
		if(FD_ISSET(udp_sock_fd, &active_sock_fd_set)){

            memset(recv_buffer, 0, MAX_PACKET_BUFFER_SIZE);
            bytes_recvd = recvfrom(udp_sock_fd, (char *)recv_buffer,
                    MAX_PACKET_BUFFER_SIZE, 0, (struct sockaddr *)&subscriber_addr, &addr_len);

            process_remote_msgs(
                    recv_buffer, bytes_recvd);
        }
    }
    return 0;
}

/*Pkt receiever thread*/
void
network_start_pkt_receiver_thread( void ){

    pthread_attr_t attr;
    pthread_t recv_pkt_thread;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_create(&recv_pkt_thread, &attr,
            _network_start_pkt_receiver_thread,
            0);
}

