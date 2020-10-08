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
#include <stdbool.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <memory.h>
#include <netdb.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <assert.h>
#include "gluethread/glthread.h"

#define MAX_PACKET_BUFFER_SIZE				1024
#define MAX_CLIENT_TCP_CONNECTION_SUPPORTED	256


typedef void (*recv_fn_cb)(char *,		/* msg recvd */
						   uint32_t,	/* recvd msg size */
						   char *,		/* Sender's IP address */
						   uint32_t,	/* Sender's Port number */
						   uint32_t,	/* Sender Communication FD , only for tcp*/
						   int []);		/* Monitored FD set array  , only for tcp*/

typedef void (*tcp_connect_cb)(char *,     /* Client's IP addr */
									 uint32_t);  /* Client's port number */

typedef void (*tcp_disconnect_cb)(char *,		/*  Client's IP addr */
								   uint32_t);  	/*  Client's port number */

/* Begin : Working with TCP Connected Clients 
 * DS to store connected TCP clients connection.
 * A TCP Connetion is uniquely identified by a
 * tuple of 4 entries : 
 * [server ip, server port , client ip, client port]
 * */
typedef struct tcp_server_{

	/* key 1 */
	int master_sock_fd;
	/* key 2 */
	char ip_addr[160];
	uint32_t port_no;

	int dummy_master_sock_fd;
	recv_fn_cb recv_fn;
	tcp_disconnect_cb tcp_disconnect_fn;
	tcp_connect_cb tcp_connect_fn;
    int monitored_tcp_fd_set_array
		[MAX_CLIENT_TCP_CONNECTION_SUPPORTED];
	pthread_t *tcp_server_thread;
	pthread_mutex_t tcp_server_pause_mutex;
	/* 	Other properties below
		< other tcp server properties >
	*/

	glthread_t clients_list_head;
	glthread_t glue;
} tcp_server_t;
GLTHREAD_TO_STRUCT(glue_to_tcp_server,
				   tcp_server_t, glue);

typedef struct tcp_connected_client_{

	/* key 1 */
	int client_comm_fd;
	/* key 2 */
	char client_ip_addr[16];
	uint32_t client_tcp_port_no;
	tcp_server_t *tcp_server; /* back pointer */
	glthread_t glue;
} tcp_connected_client_t;
GLTHREAD_TO_STRUCT(glue_to_tcp_connected_client,
				   tcp_connected_client_t, glue);

typedef struct tcp_connections_{

	glthread_t tcp_server_list_head;
} tcp_connections_db_t;

void
tcp_remove_tcp_server_entry(
	tcp_server_t *tcp_server);

void
tcp_server_add_to_db(
	tcp_connections_db_t *tcp_connections_db,
	tcp_server_t *tcp_server);

void
tcp_delete_tcp_server_entry(
	tcp_connections_db_t *tcp_connections_db,
	tcp_server_t *tcp_server);

tcp_server_t *
tcp_lookup_tcp_server_entry_by_master_fd(
	tcp_connections_db_t *tcp_connections_db,
	uint32_t master_fd);

void
tcp_create_new_tcp_connection_client_entry(
    int client_comm_fd,
    char *client_ip_addr,
    uint32_t client_tcp_port_no,
	tcp_connected_client_t *tcp_connected_client/* o/p */);

tcp_server_t *
tcp_lookup_tcp_server_entry_by_ipaddr_port(
	tcp_connections_db_t *tcp_connections_db,
	char *ip_addr,
	uint32_t port_no);

void
tcp_server_pause(tcp_server_t *tcp_server);

void
tcp_server_resume(tcp_server_t *tcp_server);

tcp_connected_client_t *
tcp_lookup_tcp_server_client_entry_by_comm_fd(
	tcp_connections_db_t *tcp_connections_db,
	uint32_t comm_fd);

tcp_connected_client_t *
tcp_lookup_tcp_server_client_entry_by_ipaddr_port(
	tcp_connections_db_t *tcp_connections_db,
	char *ip_addr,
	uint32_t port_no);

void
tcp_save_tcp_server_client_entry(
	tcp_connections_db_t *tcp_connections_db,
	uint32_t tcp_server_master_sock_fd,
	tcp_connected_client_t *tcp_connected_client);

void
tcp_delete_tcp_server_client_entry(
	tcp_connected_client_t *tcp_connected_client);

void
tcp_delete_tcp_server_client_entry_by_comm_fd(
	tcp_connections_db_t *tcp_connections_db,
	uint32_t comm_fd);

void
tcp_delete_tcp_server_client_entry_by_ipaddr_port(
	tcp_connections_db_t *tcp_connections_db,
	char *ip_addr,
	uint32_t tcp_port_no);

void
tcp_shutdown_tcp_server(
    char *server_ip_addr,
    uint32_t tcp_port_no);

void
tcp_dump_tcp_connection_db(
		tcp_connections_db_t *tcp_connections_db);

/* End : Working with TCP Connected Clients */

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
		tcp_connect_cb conn_init_req_fn,
		tcp_disconnect_cb tcp_conn_killed_fn);

int
tcp_connect(char *tcp_server_ip,
			uint32_t tcp_server_port_no);

void
tcp_fake_connect(char *tcp_server_ip,
				 uint32_t tcp_server_port_no);
void
tcp_disconnect(int comm_sock_fd,
			   char *good_bye_msg,
			   uint32_t good_bye_msg_size);

int
send_udp_msg(char *dest_ip_addr,
			 uint32_t udp_port_no,
			 char *msg,
			 uint32_t msg_size);

int
tcp_send_msg(int tcp_comm_fd,
			 char *msg,
			 uint32_t msg_size);

void
tcp_force_disconnect_client_by_ip_addr_port(
		char *ip_addr,
		uint32_t tcp_port_no);

void
tcp_force_disconnect_client_by_comm_fd(
		tcp_connections_db_t *tcp_connections_db,
		uint32_t comm_fd);

void
init_network_skt_lib(tcp_connections_db_t 
	*tcp_connections_db);

/* General Nw utilities */
char *
network_covert_ip_n_to_p(uint32_t ip_addr,
                        char *output_buffer);

uint32_t
network_covert_ip_p_to_n(char *ip_addr);

#endif /* __NETWORK_UTILS__  */
