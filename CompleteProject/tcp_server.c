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
#include "network_utils.h"


void
tcp_subscriber_join_notification(char *ip_addr,
								 uint32_t tcp_port_no){

	printf("%s() Called \n", __FUNCTION__);
}

void
tcp_subscriber_killed_notification(char *ip_addr,
								   uint32_t tcp_port_no){

	printf("%s() Called : TCP Client [%s %d] disconnected\n",
		 __FUNCTION__, ip_addr, tcp_port_no);
}

void
recv_fn( char *subs_tlv_buffer,
	 uint32_t subs_tlv_buffer_size,
	 char *subs_ip_addr,
	 uint32_t subs_port_number,
	 uint32_t subs_skt_fd) {

	printf("%s() msg recvd from %s:%u\n",
		__FUNCTION__,
		subs_ip_addr,
		htons(subs_port_number));
}

static tcp_connections_db_t tcp_connections_db;

int
main(int argc, char **argv){

	init_network_skt_lib(&tcp_connections_db);
#if 0
	udp_server_create_and_start(
			"100.1.1.2",
			40002,
			recv_fn);	
#endif
	tcp_server_create_and_start(
			"100.1.1.2",
			htons(40002),
			recv_fn,
			tcp_subscriber_join_notification,
			tcp_subscriber_killed_notification);	

	scanf("\n");
	return 0;
}

