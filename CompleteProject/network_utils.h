/*
 * =====================================================================================
 *
 *       Filename:  network_utils.h
 *
 *    Description: This file is an interface for Network Utils 
 *
 *        Version:  1.0
 *        Created:  10/06/2020 04:16:17 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ABHISHEK SAGAR (), sachinites@gmail.com
 *   Organization:  Juniper Networks
 *
 * =====================================================================================
 */

#ifndef __NETWORK_UTILS__
#define __NETWORK_UTILS__

#include <stdint.h>

#define MAX_PACKET_BUFFER_SIZE				1024
#define MAX_CLIENT_TCP_CONNECTION_SUPPORTED	256


typedef void (*recv_fn_cb)(char *,		/* msg recvd */
						   uint32_t,	/* recvd msg size */
						   char *,		/* Sender's IP address */
						   uint32_t,	/* Sender's Port number */
						   uint32_t,	/* Sender Communication FD , only for tcp*/
						   int []);		/* Monitored FD set array  , only for tcp*/

typedef void (*tcp_conn_init_req_cb)(char *,     /* Client's IP addr */
									 uint32_t);  /* Client's port number */

typedef void (*tcp_conn_killed_cb)(char *,		/*  Client's IP addr */
								   uint32_t);  	/*  Client's port number */

void
network_start_udp_pkt_receiver_thread(
		char *ip_addr,
		uint32_t udp_port_no,
		recv_fn_cb recv_fn);

void
network_start_tcp_pkt_receiver_thread(
		char *ip_addr,
		uint32_t tcp_port_no,
		recv_fn_cb recv_fn,
		tcp_conn_init_req_cb conn_init_req_fn,
		tcp_conn_killed_cb tcp_conn_killed_fn);

int
tcp_connect(char *tcp_server_ip,
			uint32_t tcp_server_port_no);


int
send_udp_msg(char *dest_ip_addr,
			 uint32_t udp_port_no,
			 char *msg,
			 uint32_t msg_size);

int
send_tcp_msg(int tcp_comm_fd,
			 char *msg,
			 uint32_t msg_size);

#endif /* __NETWORK_UTILS__  */
