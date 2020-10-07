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

#include "network_utils.h"

#define TCP_DB_MGMT

#ifdef TCP_DB_MGMT
static pthread_mutex_t tcp_db_mgmt_mutex;
#endif

/* 
 * Library holds the pointer to application instantiated
 * tcp_connections_db 
 * */
static tcp_connections_db_t *tcp_connections_db = NULL;

void
init_network_skt_lib(tcp_connections_db_t 
		*_tcp_connections_db){

	static bool initialized = false;
	
	if(initialized) return;

	initialized = true;

	tcp_connections_db = _tcp_connections_db;

	init_glthread(&tcp_connections_db->tcp_server_list_head);

	#ifdef TCP_DB_MGMT
	pthread_mutex_init(&tcp_db_mgmt_mutex, NULL);
	#endif
}


typedef struct thread_arg_pkg_{

	char ip_addr[16];
	uint32_t port_no;
	recv_fn_cb recv_fn;
	tcp_connect_cb tcp_connect_fn;
	tcp_disconnect_cb tcp_disconnect_fn;
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
	tcp_connect_cb tcp_connect_fn = thread_arg_pkg->tcp_connect_fn;
	assert(!tcp_connect_fn);

	/* When there is no connection establishment, then there can be no
 	 * connection abrupt termination (ctrl-C on client) */
	tcp_disconnect_cb tcp_tcp_disconnect_fn = thread_arg_pkg->tcp_disconnect_fn;
	assert(!tcp_tcp_disconnect_fn);
	
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
	thread_arg_pkg->tcp_connect_fn = NULL;
	thread_arg_pkg->tcp_disconnect_fn = NULL;

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

static int
remove_from_monitored_tcp_fd_set(int skt_fd,
								 int fd_arr[],
								 int fd_arr_size){

    int i = 0;

    for(; i < fd_arr_size; i++){

        if(fd_arr[i] != skt_fd)
            continue;

        fd_arr[i] = -1;
		return i;
    }
	return -1;
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
	tcp_connect_cb tcp_connect_fn = thread_arg_pkg->tcp_connect_fn;
    tcp_disconnect_cb tcp_disconnect_fn = thread_arg_pkg->tcp_disconnect_fn;
	
	free(thread_arg_pkg);
	thread_arg_pkg = NULL;

	#ifdef TCP_DB_MGMT
		tcp_server_t *tcp_server = calloc(1, sizeof(tcp_server_t));
		strncpy(tcp_server->ip_addr, ip_addr, 16);
		tcp_server->port_no = port_no;
		tcp_server->recv_fn = recv_fn;
		tcp_server->tcp_disconnect_fn = tcp_disconnect_fn;
		tcp_server->tcp_connect_fn = tcp_connect_fn;
	    monitored_tcp_fd_set_array = tcp_server->monitored_tcp_fd_set_array;
		init_glthread(&tcp_server->clients_list_head);
		init_glthread(&tcp_server->glue);
		pthread_mutex_lock(&tcp_db_mgmt_mutex);
		tcp_server_add_to_db(tcp_connections_db, tcp_server);
		pthread_mutex_unlock(&tcp_db_mgmt_mutex);
	#else
		monitored_tcp_fd_set_array = calloc(MAX_CLIENT_TCP_CONNECTION_SUPPORTED,
											 sizeof(int));	
	#endif

	intitialize_monitored_fd_set(monitored_tcp_fd_set_array,
								 MAX_CLIENT_TCP_CONNECTION_SUPPORTED);
	
	int tcp_master_sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP );
	int tcp_dummy_master_sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP );

	if(tcp_master_sock_fd == -1 || 
		tcp_dummy_master_sock_fd == -1){
		printf("Socket Creation Failed\n");
		goto CLEANUP;
	}

	struct sockaddr_in server_addr;
	server_addr.sin_family      = AF_INET;
	server_addr.sin_port        = port_no;
	server_addr.sin_addr.s_addr = INADDR_ANY;

    if (setsockopt(tcp_master_sock_fd, SOL_SOCKET, 
				   SO_REUSEADDR, (char *)&opt, sizeof(opt))<0) {
		printf("setsockopt Failed\n");
		goto CLEANUP;
    }
    
	if (setsockopt(tcp_master_sock_fd, SOL_SOCKET,
				   SO_REUSEPORT, (char *)&opt, sizeof(opt))<0) {
		printf("setsockopt Failed\n");
		goto CLEANUP;
    }

	if (bind(tcp_master_sock_fd, (struct sockaddr *)&server_addr,
				sizeof(struct sockaddr)) == -1) {
		printf("Error : socket bind failed\n");
		goto CLEANUP;
	}

	if (listen(tcp_master_sock_fd, 5) < 0 ) {
		
		printf("listen failed\n");
		goto CLEANUP;
	}
			
	server_addr.sin_port        = port_no + 1;

    if (setsockopt(tcp_dummy_master_sock_fd, SOL_SOCKET, 
				   SO_REUSEADDR, (char *)&opt, sizeof(opt))<0) {
		printf("setsockopt Failed\n");
		goto CLEANUP;
    }
    
	if (setsockopt(tcp_dummy_master_sock_fd, SOL_SOCKET,
				   SO_REUSEPORT, (char *)&opt, sizeof(opt))<0) {
		printf("setsockopt Failed\n");
		goto CLEANUP;
    }

	if (bind(tcp_dummy_master_sock_fd, (struct sockaddr *)&server_addr,
				sizeof(struct sockaddr)) == -1) {
		printf("Error : socket bind failed\n");
		goto CLEANUP;
	}

	if (listen(tcp_dummy_master_sock_fd, 5) < 0 ) {
		
		printf("listen failed\n");
		goto CLEANUP;
	}
	
	add_to_monitored_tcp_fd_set(tcp_master_sock_fd,
								monitored_tcp_fd_set_array,
								MAX_CLIENT_TCP_CONNECTION_SUPPORTED);
	
	add_to_monitored_tcp_fd_set(tcp_dummy_master_sock_fd,
								monitored_tcp_fd_set_array,
								MAX_CLIENT_TCP_CONNECTION_SUPPORTED);

	#ifdef TCP_DB_MGMT
		tcp_server->master_sock_fd = tcp_master_sock_fd;
		tcp_server->dummy_master_sock_fd = tcp_dummy_master_sock_fd;
	#endif

	char *recv_buffer = calloc(1, MAX_PACKET_BUFFER_SIZE);

	fd_set active_sock_fd_set,
    	   backup_sock_fd_set;

    FD_ZERO(&active_sock_fd_set);
    FD_ZERO(&backup_sock_fd_set);

    FD_SET(tcp_master_sock_fd, &backup_sock_fd_set);
    FD_SET(tcp_dummy_master_sock_fd, &backup_sock_fd_set);

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

			#ifdef TCP_DB_MGMT

			tcp_connected_client_t *tcp_connected_client = 
					calloc(1, sizeof(tcp_connected_client_t));

			tcp_create_new_tcp_connection_client_entry(
					comm_socket_fd,
					network_covert_ip_n_to_p((uint32_t)client_addr.sin_addr.s_addr, 0), 
					client_addr.sin_port, tcp_connected_client);
			
			pthread_mutex_lock(&tcp_db_mgmt_mutex);
			tcp_save_tcp_server_client_entry(
				tcp_connections_db,
				tcp_master_sock_fd,
				tcp_connected_client);
			pthread_mutex_unlock(&tcp_db_mgmt_mutex);

			#endif

			if(tcp_connect_fn) tcp_connect_fn( 0, 0);
        }
		else if(FD_ISSET(tcp_dummy_master_sock_fd, &active_sock_fd_set)){
			
			fd_set_skt_fd(&backup_sock_fd_set,
						  monitored_tcp_fd_set_array,
						  MAX_CLIENT_TCP_CONNECTION_SUPPORTED);
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

						if(tcp_disconnect_fn) tcp_disconnect_fn(0, 0);
						
						#ifdef TCP_DB_MGMT
							pthread_mutex_lock(&tcp_db_mgmt_mutex);
							tcp_connected_client_t *tcp_connected_client = 
								tcp_lookup_tcp_server_client_entry_by_comm_fd(
									tcp_connections_db, comm_socket_fd);
							assert(tcp_connected_client);
							tcp_delete_tcp_server_client_entry(tcp_connected_client);		
							pthread_mutex_unlock(&tcp_db_mgmt_mutex);
						#endif

					} 
					else {
						recv_fn(recv_buffer, bytes_recvd, 0, 0, 0, 0);					
					}
				}
			}
		}
    }
	CLEANUP:
	#ifdef TCP_DB_MGMT
	 	pthread_mutex_lock(&tcp_db_mgmt_mutex);
		tcp_remove_tcp_server_entry(tcp_server);
		pthread_mutex_unlock(&tcp_db_mgmt_mutex);
		free(tcp_server);	
	#endif
    return 0;
}


void
network_start_tcp_pkt_receiver_thread(
        char *ip_addr,
        uint32_t tcp_port_no,
		recv_fn_cb recv_fn,
		tcp_connect_cb tcp_connect_fn,
	    tcp_disconnect_cb tcp_disconnect_fn) {

    pthread_attr_t attr;
    pthread_t recv_pkt_thread;
	thread_arg_pkg_t *thread_arg_pkg;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	thread_arg_pkg = calloc(1, sizeof(thread_arg_pkg_t));
	strncpy(thread_arg_pkg->ip_addr, ip_addr, 16);
	thread_arg_pkg->port_no = tcp_port_no;
	thread_arg_pkg->recv_fn = recv_fn;
	thread_arg_pkg->tcp_connect_fn = tcp_connect_fn;
	thread_arg_pkg->tcp_disconnect_fn = tcp_disconnect_fn;

    pthread_create(&recv_pkt_thread, &attr,
			_network_start_tcp_pkt_receiver_thread,
            (void *)thread_arg_pkg);
}


int
tcp_send_msg(int tcp_comm_fd,
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

void
tcp_fake_connect(char *tcp_server_ip,
	  			 uint32_t tcp_server_port_no){

	int rc = 0;
	int sock_fd = -1;

	if ((sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
		return;
	}

	struct hostent *host;
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));

	host = (struct hostent *)gethostbyname(tcp_server_ip);
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons((unsigned short)tcp_server_port_no);
	server_addr.sin_addr = *((struct in_addr *)host->h_addr);

	rc = connect(sock_fd, (struct sockaddr *)&server_addr,sizeof(struct sockaddr));

	close(sock_fd);	
}

void
tcp_disconnect(int comm_sock_fd,
               char *good_bye_msg,
               uint32_t good_bye_msg_size) {

	if(good_bye_msg && good_bye_msg_size > 0 ) {
		tcp_send_msg(comm_sock_fd, good_bye_msg, good_bye_msg_size);
	}

	close(comm_sock_fd);
}


#ifdef TCP_DB_MGMT

tcp_connected_client_t *
tcp_lookup_tcp_server_client_entry_by_comm_fd(
	tcp_connections_db_t *tcp_connections_db,
	uint32_t comm_fd){

	glthread_t *curr, *curr2;
	tcp_server_t *tcp_server_curr;
	tcp_connected_client_t *tcp_connected_client_curr;

	ITERATE_GLTHREAD_BEGIN(&tcp_connections_db->tcp_server_list_head, curr){

		tcp_server_curr = glue_to_tcp_server(curr);

		ITERATE_GLTHREAD_BEGIN(&tcp_server_curr->clients_list_head, curr2){

			tcp_connected_client_curr = glue_to_tcp_connected_client(curr2);

			if(tcp_connected_client_curr->client_comm_fd == comm_fd) {
				
				return tcp_connected_client_curr;
			}
		}ITERATE_GLTHREAD_END(&tcp_server_curr->clients_list_head, curr2);				

	} ITERATE_GLTHREAD_END(&tcp_connections_db->tcp_server_list_head, curr);
	return NULL;
}

void
tcp_delete_tcp_server_client_entry(
	tcp_connected_client_t *tcp_connected_client){

	remove_glthread(&tcp_connected_client->glue);
	tcp_connected_client->tcp_server = NULL;
	free(tcp_connected_client);
}


void
tcp_force_disconnect_client_by_comm_fd(
	tcp_connections_db_t *tcp_connections_db,
	uint32_t comm_fd){

	tcp_connected_client_t * tcp_connected_client = 
		tcp_lookup_tcp_server_client_entry_by_comm_fd(
			tcp_connections_db, comm_fd);

	if ( !tcp_connected_client) return;

	tcp_server_t *tcp_server = tcp_connected_client->tcp_server;
	
	assert(tcp_server);

	assert((remove_from_monitored_tcp_fd_set(
			tcp_connected_client->client_comm_fd,
			tcp_server->monitored_tcp_fd_set_array,
			sizeof(tcp_server->monitored_tcp_fd_set_array)/\
				sizeof(tcp_server->monitored_tcp_fd_set_array[0]))>= 0));

	tcp_fake_connect(tcp_server->ip_addr, tcp_server->port_no + 1);
	
	tcp_delete_tcp_server_client_entry(tcp_connected_client);
}

void
tcp_create_new_tcp_connection_client_entry(
    int client_comm_fd,
    char *client_ip_addr,
    uint32_t client_tcp_port_no,
    tcp_connected_client_t *tcp_connected_client/*  o/p */) {

	tcp_connected_client->client_comm_fd = client_comm_fd;
	strncpy(tcp_connected_client->client_ip_addr,
			client_ip_addr, 16);
	tcp_connected_client->client_tcp_port_no = client_tcp_port_no;
	tcp_connected_client->tcp_server = NULL;
	init_glthread(&tcp_connected_client->glue);
}

void
tcp_save_tcp_server_client_entry(
    tcp_connections_db_t *tcp_connections_db,
    uint32_t tcp_server_master_sock_fd,
    tcp_connected_client_t *tcp_connected_client){

	tcp_server_t *tcp_server = tcp_lookup_tcp_server_entry_by_master_fd(
								tcp_connections_db,
						        tcp_server_master_sock_fd);

	assert(tcp_server);

	assert(!tcp_connected_client->tcp_server);

	tcp_connected_client->tcp_server = tcp_server;

	init_glthread(&tcp_connected_client->glue);

	glthread_add_next(&tcp_server->clients_list_head, &tcp_connected_client->glue);
}

tcp_server_t *
tcp_lookup_tcp_server_entry_by_master_fd(
    tcp_connections_db_t *tcp_connections_db,
    uint32_t master_fd){

	glthread_t *curr;
	tcp_server_t *tcp_server_curr;

	ITERATE_GLTHREAD_BEGIN(&tcp_connections_db->tcp_server_list_head, curr){

		tcp_server_curr = glue_to_tcp_server(curr);

		if(tcp_server_curr->master_sock_fd == master_fd)
			return tcp_server_curr;

	} ITERATE_GLTHREAD_END(&tcp_connections_db->tcp_server_list_head, curr);
	return NULL;
}

static void
print_tcp_connected_client(
		tcp_connected_client_t *tcp_connected_client){

	printf("\tClient : [%s %u]\n", tcp_connected_client->client_ip_addr,
								   tcp_connected_client->client_tcp_port_no);
}

static void
print_tcp_server_info(tcp_server_t *tcp_server){

	glthread_t *curr;
	tcp_connected_client_t *tcp_connected_client;

	printf("TCP Server : [%s %u]\n", tcp_server->ip_addr, tcp_server->port_no);

	ITERATE_GLTHREAD_BEGIN(&tcp_server->clients_list_head, curr){

		tcp_connected_client = glue_to_tcp_connected_client(curr);
		print_tcp_connected_client(tcp_connected_client);

	} ITERATE_GLTHREAD_END(&tcp_server->clients_list_head, curr);	
}

void
tcp_dump_tcp_connection_db(
        tcp_connections_db_t *tcp_connections_db){


	glthread_t *curr;
	tcp_server_t *tcp_server_curr;

	ITERATE_GLTHREAD_BEGIN(&tcp_connections_db->tcp_server_list_head, curr){

		tcp_server_curr = glue_to_tcp_server(curr);

		print_tcp_server_info(tcp_server_curr);

	} ITERATE_GLTHREAD_END(&tcp_connections_db->tcp_server_list_head, curr);
}

void
tcp_remove_tcp_server_entry(
    tcp_server_t *tcp_server){

	assert(IS_GLTHREAD_LIST_EMPTY(&tcp_server->clients_list_head));	
	remove_glthread(&tcp_server->glue);
}

void
tcp_server_add_to_db(
    tcp_connections_db_t *tcp_connections_db,
    tcp_server_t *tcp_server) {

	assert(IS_GLTHREAD_LIST_EMPTY(&tcp_server->glue));

	assert(!tcp_lookup_tcp_server_entry_by_master_fd(
				tcp_connections_db, tcp_server->master_sock_fd));

	glthread_add_next(&tcp_connections_db->tcp_server_list_head,
					  &tcp_server->glue);
}


#endif

char *
network_covert_ip_n_to_p(uint32_t ip_addr,
                    char *output_buffer){

    char *out = NULL;
    static char str_ip[16];
    out = !output_buffer ? str_ip : output_buffer;
    memset(out, 0, 16);
    ip_addr = htonl(ip_addr);
    inet_ntop(AF_INET, &ip_addr, out, 16);
    out[15] = '\0';
    return out;
}

uint32_t
network_covert_ip_p_to_n(char *ip_addr){

    uint32_t binary_prefix = 0;
    inet_pton(AF_INET, ip_addr, &binary_prefix);
    binary_prefix = htonl(binary_prefix);
    return binary_prefix;
}
