/*
 * =====================================================================================
 *
 *       Filename:  network_utils.c
 *
 *    Description: This file contains routines to work with Network sockets programs 
 *
 *        Version:  1.0
 *        Created:  10/06/2020 04:15:57 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ABHISHEK SAGAR (), sachinites@gmail.com
 *   Organization:  Juniper Networks
 *
 * =====================================================================================
 */

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
#include "network_utils.h"

typedef struct thread_arg_pkg_{

	char ip_addr[16];
	uint32_t port_no;
	recv_fn_cb recv_fn;
	tcp_conn_init_req_cb conn_init_req_fn;
	tcp_conn_killed_cb conn_killed_fn;
} thread_arg_pkg_t;


/* UDP Server code*/

static void *
_network_start_udp_pkt_receiver_thread(void *arg){

	thread_arg_pkg_t *thread_arg_pkg = 
		(thread_arg_pkg_t *)arg;

	char *ip_addr 	   = thread_arg_pkg->ip_addr;
	uint32_t port_no   = thread_arg_pkg->port_no;
	recv_fn_cb recv_fn = thread_arg_pkg->recv_fn;
	
	/* Not applicable for UDP communication as in
 	 * UDP communication there is really no Connection Establishment */
	tcp_conn_init_req_cb conn_init_req_fn = thread_arg_pkg->conn_init_req_fn;
	assert(!conn_init_req_fn);

	/* When there is no connection establishment, then there can be no
 	 * connection abrupt termination (ctrl-C on client) */
	tcp_conn_killed_cb tcp_conn_killed_fn = thread_arg_pkg->conn_killed_fn;
	assert(!tcp_conn_killed_fn);
	
	free(thread_arg_pkg);
	thread_arg_pkg = NULL;
	
	int udp_sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP );

	if(udp_sock_fd == -1){
		printf("Socket Creation Failed\n");
		return 0;
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family      = AF_INET;
	server_addr.sin_port        = port_no;
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(udp_sock_fd, (struct sockaddr *)&server_addr,
				sizeof(struct sockaddr)) == -1) {
		printf("Error : socket bind failed\n");
		return 0;
	}

	char *recv_buffer = calloc(1, MAX_PACKET_BUFFER_SIZE);

	fd_set active_sock_fd_set,
    backup_sock_fd_set;

    FD_ZERO(&active_sock_fd_set);
    FD_ZERO(&backup_sock_fd_set);

    struct sockaddr_in client_addr;
    FD_SET(udp_sock_fd, &backup_sock_fd_set);
    int bytes_recvd = 0,
       	addr_len = sizeof(client_addr);

    while(1){

        memcpy(&active_sock_fd_set, &backup_sock_fd_set, sizeof(fd_set));
        select(udp_sock_fd + 1, &active_sock_fd_set, NULL, NULL, NULL);

        if(FD_ISSET(udp_sock_fd, &active_sock_fd_set)){

            memset(recv_buffer, 0, MAX_PACKET_BUFFER_SIZE);
            bytes_recvd = recvfrom(udp_sock_fd, recv_buffer,
                    MAX_PACKET_BUFFER_SIZE, 0, 
					(struct sockaddr *)&client_addr, &addr_len);

            recv_fn(recv_buffer, bytes_recvd, 0, 0, 0, 0);
        }
    }
    return 0;
}


void
network_start_udp_pkt_receiver_thread(
        char *ip_addr,
        uint32_t udp_port_no,
		recv_fn_cb recv_fn){

    pthread_attr_t attr;
    pthread_t recv_pkt_thread;
	thread_arg_pkg_t *thread_arg_pkg;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	thread_arg_pkg = calloc(1, sizeof(thread_arg_pkg_t));
	strncpy(thread_arg_pkg->ip_addr, ip_addr, 16);
	thread_arg_pkg->port_no = udp_port_no;
	thread_arg_pkg->recv_fn = recv_fn;
	thread_arg_pkg->conn_init_req_fn = NULL;
	thread_arg_pkg->conn_killed_fn = NULL;

    pthread_create(&recv_pkt_thread, &attr,
			_network_start_udp_pkt_receiver_thread,
            (void *)thread_arg_pkg);
}

int
send_udp_msg(char *dest_ip_addr,
             uint32_t dest_port_no,
             char *msg,
             uint32_t msg_size) {
    
	struct sockaddr_in dest;

    dest.sin_family = AF_INET;
    dest.sin_port = dest_port_no;
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


/* TCP Server Code */

/* Begin : Monitored Socket FD Array operations */
static void
intitialize_monitored_fd_set(int fd_arr[], int fd_arr_size){

	int i = 0 ;
    for(; i < fd_arr_size; i++)
        fd_arr[i] = -1;
}

static void
add_to_monitored_tcp_fd_set(int skt_fd, 
							int fd_arr[],
							int fd_arr_size){

    int i = 0;

    for(; i < fd_arr_size; i++){

        if(fd_arr[i] != -1)
            continue;
        fd_arr[i] = skt_fd;
        break;
    }
}

static void
remove_from_monitored_tcp_fd_set(int skt_fd,
								 int fd_arr[],
								 int fd_arr_size){

    int i = 0;

    for(; i < fd_arr_size; i++){

        if(fd_arr[i] != skt_fd)
            continue;

        fd_arr[i] = -1;
        break;
    }
}

static int
get_max_fd(int fd_arr[], int fd_arr_size){

    int i = 0;
    int max = -1;

    for(; i < fd_arr_size; i++){
        if(fd_arr[i] > max)
            max = fd_arr[i];
    }

    return max;
}

static void
fd_set_skt_fd(fd_set *_fd_set, 
			  int fd_arr[],
			  int fd_arr_size) {

	int i = 0;

	FD_ZERO(_fd_set);
	
	for( ; i < fd_arr_size; i++){
		
		if(fd_arr[i] == -1) continue;
		FD_SET(fd_arr[i], _fd_set);
	}		
}

/* End : Monitored Socket FD Array operations */



static void *
_network_start_tcp_pkt_receiver_thread(void *arg){

	int opt = 1;
	int *monitored_tcp_fd_set_array;

	thread_arg_pkg_t *thread_arg_pkg = 
		(thread_arg_pkg_t *)arg;

	char *ip_addr 	   = thread_arg_pkg->ip_addr;
	uint32_t port_no   = thread_arg_pkg->port_no;
	recv_fn_cb recv_fn = thread_arg_pkg->recv_fn;
	tcp_conn_init_req_cb conn_init_req_fn = thread_arg_pkg->conn_init_req_fn;
    tcp_conn_killed_cb conn_killed_fn = thread_arg_pkg->conn_killed_fn;
	
	free(thread_arg_pkg);
	thread_arg_pkg = NULL;

	monitored_tcp_fd_set_array = calloc(MAX_CLIENT_TCP_CONNECTION_SUPPORTED,
								 sizeof(int));

	intitialize_monitored_fd_set(monitored_tcp_fd_set_array,
								 MAX_CLIENT_TCP_CONNECTION_SUPPORTED);
	
	int tcp_master_sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP );

	if(tcp_master_sock_fd == -1){
		printf("Socket Creation Failed\n");
		return 0;
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family      = AF_INET;
	server_addr.sin_port        = port_no;
	server_addr.sin_addr.s_addr = INADDR_ANY;

    if (setsockopt(tcp_master_sock_fd, SOL_SOCKET, 
				   SO_REUSEADDR, (char *)&opt, sizeof(opt))<0) {
		printf("setsockopt Failed\n");
		return 0;
    }
    
	if (setsockopt(tcp_master_sock_fd, SOL_SOCKET,
				   SO_REUSEPORT, (char *)&opt, sizeof(opt))<0) {
		printf("setsockopt Failed\n");
		return 0;
    }

	if (bind(tcp_master_sock_fd, (struct sockaddr *)&server_addr,
				sizeof(struct sockaddr)) == -1) {
		printf("Error : socket bind failed\n");
		return 0;
	}

	if (listen(tcp_master_sock_fd, 5) < 0 ) {
		
		printf("listen failed\n");
		return 0;
	}
			
	add_to_monitored_tcp_fd_set(tcp_master_sock_fd,
								monitored_tcp_fd_set_array,
								MAX_CLIENT_TCP_CONNECTION_SUPPORTED);

	char *recv_buffer = calloc(1, MAX_PACKET_BUFFER_SIZE);

	fd_set active_sock_fd_set,
    	   backup_sock_fd_set;

    FD_ZERO(&active_sock_fd_set);
    FD_ZERO(&backup_sock_fd_set);

    FD_SET(tcp_master_sock_fd, &backup_sock_fd_set);

    struct sockaddr_in client_addr;
    int bytes_recvd = 0,
       	addr_len = sizeof(client_addr);

    while(1){

        memcpy(&active_sock_fd_set, &backup_sock_fd_set, sizeof(fd_set));
        select(
			get_max_fd (monitored_tcp_fd_set_array,
					    MAX_CLIENT_TCP_CONNECTION_SUPPORTED) + 1,
			&active_sock_fd_set, NULL, NULL, NULL);

        if(FD_ISSET(tcp_master_sock_fd, &active_sock_fd_set)){

			/* Connection initiation Request */
			int comm_socket_fd =  accept(tcp_master_sock_fd,
				 					     (struct sockaddr *)&client_addr,
					   					 &addr_len);
			if(comm_socket_fd < 0 ){
				continue;
			}

		    add_to_monitored_tcp_fd_set(comm_socket_fd,
										monitored_tcp_fd_set_array,
										MAX_CLIENT_TCP_CONNECTION_SUPPORTED);

			fd_set_skt_fd(&backup_sock_fd_set,
						  monitored_tcp_fd_set_array,
						  MAX_CLIENT_TCP_CONNECTION_SUPPORTED);

			if(conn_init_req_fn) conn_init_req_fn( 0, 0);
        }
		else {
			int i = 0;
			for ( i = 0; i < MAX_CLIENT_TCP_CONNECTION_SUPPORTED; i++){
				
				if(monitored_tcp_fd_set_array[i] == -1 || 
				   monitored_tcp_fd_set_array[i] == tcp_master_sock_fd) {
						continue;
				}

				if(FD_ISSET(monitored_tcp_fd_set_array[i], &active_sock_fd_set)){
					
					/* Data Request from existing connection */
					int comm_socket_fd = monitored_tcp_fd_set_array[i];

					memset(recv_buffer, 0 , MAX_PACKET_BUFFER_SIZE);
					
					bytes_recvd = recvfrom(comm_socket_fd, recv_buffer,
								  MAX_PACKET_BUFFER_SIZE, 0,
								  (struct sockaddr *)&client_addr, &addr_len);
					
					/* The connected client has Cored/Crashed/Seg fault or
 					 * or abruptly terminated for other reasons such as Ctrl-C */			
					if(bytes_recvd == 0) {
						
						remove_from_monitored_tcp_fd_set(comm_socket_fd,
											monitored_tcp_fd_set_array,
											MAX_CLIENT_TCP_CONNECTION_SUPPORTED);
						
						fd_set_skt_fd(&backup_sock_fd_set,
						  monitored_tcp_fd_set_array,
						  MAX_CLIENT_TCP_CONNECTION_SUPPORTED);

						if(conn_killed_fn) conn_killed_fn(0, 0);
					} 
					else {
						recv_fn(recv_buffer, bytes_recvd, 0, 0, 0, 0);					
					}
				}
			}
		}
    }
    return 0;
}


void
network_start_tcp_pkt_receiver_thread(
        char *ip_addr,
        uint32_t tcp_port_no,
		recv_fn_cb recv_fn,
		tcp_conn_init_req_cb conn_init_req_fn,
	    tcp_conn_killed_cb conn_killed_fn) {

    pthread_attr_t attr;
    pthread_t recv_pkt_thread;
	thread_arg_pkg_t *thread_arg_pkg;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	thread_arg_pkg = calloc(1, sizeof(thread_arg_pkg_t));
	strncpy(thread_arg_pkg->ip_addr, ip_addr, 16);
	thread_arg_pkg->port_no = tcp_port_no;
	thread_arg_pkg->recv_fn = recv_fn;
	thread_arg_pkg->conn_init_req_fn = conn_init_req_fn;
	thread_arg_pkg->conn_killed_fn = conn_killed_fn;

    pthread_create(&recv_pkt_thread, &attr,
			_network_start_tcp_pkt_receiver_thread,
            (void *)thread_arg_pkg);
}


int
send_tcp_msg(int tcp_comm_fd,
             char *msg,
             uint32_t msg_size) {

	return  sendto(tcp_comm_fd, msg, msg_size, 0, NULL, 0);    
}

/* Return +ve sock fd on successfull connection
 * else return -1 */
int
tcp_connect(char *tcp_server_ip,
            uint32_t tcp_server_port_no){

	int rc = 0;
	int sock_fd = -1;

	if ((sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
		return -1;
	}

	struct hostent *host;
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));

	host = (struct hostent *)gethostbyname(tcp_server_ip);
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons((unsigned short)tcp_server_port_no);
	server_addr.sin_addr = *((struct in_addr *)host->h_addr);

	rc = connect(sock_fd, (struct sockaddr *)&server_addr,sizeof(struct sockaddr));
	
	if( rc < 0) {
		close(sock_fd);
		return -1;
	}
	
	return sock_fd;
}
