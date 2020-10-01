/*
 * =====================================================================================
 *
 *       Filename:  notif.c
 *
 *    Description:  This file implements the routines and fns required to implement Notification Chains
 *
 *        Version:  1.0
 *        Created:  08/23/2020 08:19:58 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Er. Abhishek Sagar, Juniper Networks (https://csepracticals.wixsite.com/csepracticals), sachinites@gmail.com
 *        Company:  Juniper Networks
 *
 *        This file is part of the NotificationChains distribution (https://github.com/sachinites) 
 *        Copynext (c) 2019 Abhishek Sagar.
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

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h> // for close
#include <netdb.h>  /*for struct hostent*/
#include "notif.h"
#include "utils.h"

static notif_chain_db_t notif_chain_db = {0};

static void 
notif_chain_register_notif_chain(notif_chain_t *notif_chain){

	notif_chain_t *notif_chain_head = notif_chain_db.notif_chain_head;
	notif_chain_db.notif_chain_head = notif_chain;
	notif_chain->prev = 0;
	notif_chain->next = notif_chain_head;
	if(notif_chain_head)
		notif_chain_head->prev = notif_chain;	
}

static notif_chain_t *
notif_chain_lookup_notif_chain_by_name(char *notif_chain_name){

	notif_chain_t *notif_chain = notif_chain_db.notif_chain_head;
	for( ; notif_chain; notif_chain = notif_chain->next){
		if(strncmp(notif_chain->name, notif_chain_name, NOTIF_NAME_SIZE) == 0)
			return notif_chain;
	}
	return 0;
}


void
notif_chain_init(notif_chain_t *notif_chain,
		char *chain_name,
		notif_chain_comp_cb comp_cb,
		app_key_data_print_cb print_cb){

	struct sockaddr_in notif_chain_addr;

	memset(notif_chain, 0, sizeof(notif_chain_t));
	strncpy(notif_chain->name, chain_name, sizeof(notif_chain->name));
	notif_chain->name[sizeof(notif_chain->name) -1] = '\0';
	notif_chain->comp_cb = comp_cb;
	notif_chain->print_cb = print_cb;
	notif_chain->next = 0;
	notif_chain->prev = 0;
	notif_chain_register_notif_chain(notif_chain);
}

void
notif_chain_release_communication_channel_resources(
		notif_chain_comm_channel_t *channel){

	notif_ch_type_t notif_ch_type;

	notif_ch_type = channel->notif_ch_type;

	switch(notif_ch_type){

		case NOTIF_C_CALLBACKS:
			return;
		case NOTIF_C_MSG_Q:
			/* Release Msg Q resources here*/
			return;
		case NOTIF_C_AF_UNIX:
			/*Release UNIX Sockets Resources here*/
			return;
		case NOTIF_C_INET_SOCKETS:
			/*Release INET Sockets reources here*/
			return;
		case NOTIF_C_ANY:
		case NOTIF_C_NOT_KNOWN:
		default:    ;
	}
}

void
notif_chain_free_notif_chain_elem_internals(
		notif_chain_elem_t *notif_chain_elem){

	assert(notif_chain_elem->prev == 0 && 
			notif_chain_elem->next == 0);

	if(notif_chain_elem->data.app_key_data){
		free(notif_chain_elem->data.app_key_data);
	}
	notif_chain_elem->data.app_key_data = 0;
	notif_chain_elem->data.app_key_data_size = 0;

	if(notif_chain_elem->data.is_alloc_app_data_to_notify){
		free(notif_chain_elem->data.app_data_to_notify);
	}
	notif_chain_elem->data.app_data_to_notify = 0;
	notif_chain_elem->data.app_data_to_notify_size = 0;
	notif_chain_release_communication_channel_resources(
			&notif_chain_elem->notif_chain_comm_channel);
}

void
notif_chain_free_notif_chain_elem(
		notif_chain_elem_t *notif_chain_elem){

	notif_chain_free_notif_chain_elem_internals(
			notif_chain_elem);

	free(notif_chain_elem); 
}


void
notif_chain_delete(notif_chain_t *notif_chain){

	notif_chain_elem_t *notif_chain_elem;

	ITERTAE_NOTIF_CHAIN_BEGIN(notif_chain, notif_chain_elem){

		notif_chain_elem_remove(notif_chain, notif_chain_elem);
		notif_chain_free_notif_chain_elem(notif_chain_elem);
	} ITERTAE_NOTIF_CHAIN_END(notif_chain, notif_chain_elem);
	notif_chain->head = NULL;
}

bool
notif_chain_register_chain_element(notif_chain_t *notif_chain,
		notif_chain_elem_t *notif_chain_elem,
		bool request_malloc){

	notif_chain_elem_t *new_notif_chain_elem ;

    if (notif_chain_lookup_notif_chain_element(
						notif_chain,
						notif_chain_elem->client_id,
						notif_chain_elem->data.app_key_data,
						notif_chain_elem->data.app_key_data_size)) {

		return false;
	}

	if (request_malloc)	
	new_notif_chain_elem =  (notif_chain_elem_t *)calloc(1,sizeof(notif_chain_elem_t));
	else
	new_notif_chain_elem = notif_chain_elem;

	if(!new_notif_chain_elem) return false;

	memcpy(new_notif_chain_elem, notif_chain_elem, 
			sizeof(notif_chain_elem_t));

	if(notif_chain_elem->data.app_key_data){
		new_notif_chain_elem->data.app_key_data = 
			calloc(1, notif_chain_elem->data.app_key_data_size);
		memcpy(new_notif_chain_elem->data.app_key_data,
				notif_chain_elem->data.app_key_data, 
				notif_chain_elem->data.app_key_data_size);
		new_notif_chain_elem->data.app_key_data_size = 
			notif_chain_elem->data.app_key_data_size;
	}
	notif_chain_elem_t *head = notif_chain->head;
	notif_chain->head = new_notif_chain_elem;
	new_notif_chain_elem->prev = 0;
	new_notif_chain_elem->next = head;
	if(head)
		head->prev = new_notif_chain_elem;
	return true;
}

bool
notif_chain_communication_channel_match(
		notif_chain_comm_channel_t *channel1,
		notif_chain_comm_channel_t *channel2){

	if(channel1->notif_ch_type != channel2->notif_ch_type)
		return false;

	notif_ch_type_t notif_ch_type = channel1->notif_ch_type;

	switch(notif_ch_type){

		case NOTIF_C_ANY:
			return true;
		case NOTIF_C_CALLBACKS:
			if(NOTIF_CHAIN_ELEM_APP_CB(channel1) != 
					NOTIF_CHAIN_ELEM_APP_CB(channel2))
				return false;
			return true;
		case NOTIF_C_MSG_Q:
			if(strncmp(NOTIF_CHAIN_ELEM_MSGQ_NAME(channel1),
						NOTIF_CHAIN_ELEM_MSGQ_NAME(channel2),
						NOTIF_C_COMM_CHANNEL_MSGQ_NAME_VALUE_LEN))
				return false;
			return true;
		case NOTIF_C_AF_UNIX:
			if(strncmp(NOTIF_CHAIN_ELEM_SKT_NAME(channel1),
						NOTIF_CHAIN_ELEM_SKT_NAME(channel2),
						NOTIF_C_COMM_CHANNEL_UNIX_SKT_NAME_VALUE_LEN))
				return false;
			return true;
		case NOTIF_C_INET_SOCKETS:
			if((NOTIF_CHAIN_ELEM_IP_ADDR(channel1) == 
						NOTIF_CHAIN_ELEM_IP_ADDR(channel2)) 
					&&
					(NOTIF_CHAIN_ELEM_PORT_NO(channel1) ==
					 NOTIF_CHAIN_ELEM_PORT_NO(channel2))
					&&
					(NOTIF_CHAIN_ELEM_PROTO(channel1) == 
					 NOTIF_CHAIN_ELEM_PROTO(channel2)))
				return true;
			return false;
		case NOTIF_C_NOT_KNOWN:
			return true;
		default:
			return true;
	}
	return true;
}

bool
notif_chain_deregister_chain_element(notif_chain_t *notif_chain,
		notif_chain_elem_t *notif_chain_elem){

	bool is_removed = false;
	notif_chain_elem_t *notif_chain_elem_curr;
	notif_chain_comm_channel_t *notif_chain_comm_channel;

	notif_chain_comm_channel = &notif_chain_elem->notif_chain_comm_channel;

	ITERTAE_NOTIF_CHAIN_BEGIN(notif_chain, notif_chain_elem_curr){

		if(notif_chain_elem_curr->client_id != 
				notif_chain_elem->client_id){

			continue;
		}

		if(notif_chain->comp_cb &&
				notif_chain->comp_cb(
					notif_chain_elem_curr->data.app_key_data,
					notif_chain_elem_curr->data.app_key_data_size,
					notif_chain_elem->data.app_key_data, 
					notif_chain_elem->data.app_key_data_size) != 0){

			continue;
		}

		/*Compare connunication channel also*/
		if(notif_chain_comm_channel->notif_ch_type != NOTIF_C_ANY){

			if(notif_chain_comm_channel->notif_ch_type != 
					notif_chain_elem_curr->notif_chain_comm_channel.notif_ch_type){
				continue;
			}

			/*Now match Notification channel exactness*/
			if(!notif_chain_communication_channel_match(
						notif_chain_comm_channel,
						&notif_chain_elem_curr->notif_chain_comm_channel)){

				continue;
			}
		}

		notif_chain_elem_remove(notif_chain, notif_chain_elem_curr);
		notif_chain_free_notif_chain_elem(notif_chain_elem_curr);
		is_removed = true;

	} ITERTAE_NOTIF_CHAIN_END(notif_chain, notif_chain_elem_curr);
	return is_removed;
}

static void
notif_chain_invoke_communication_channel(
		notif_chain_t *notif_chain,
		notif_chain_elem_t *notif_chain_elem){

	notif_chain_comm_channel_t *
		notif_chain_comm_channel = &notif_chain_elem->notif_chain_comm_channel;

	switch(notif_chain_comm_channel->notif_ch_type){

		case NOTIF_C_CALLBACKS:
			assert(NOTIF_CHAIN_ELEM_APP_CB(notif_chain_comm_channel));
			NOTIF_CHAIN_ELEM_APP_CB(notif_chain_comm_channel)(
					notif_chain_elem);
			break;
		case NOTIF_C_MSG_Q:
			break;
		case NOTIF_C_AF_UNIX:
			break;
		case NOTIF_C_INET_SOCKETS:
			break;
		case NOTIF_C_NOT_KNOWN:
			break;
		case NOTIF_C_ANY:
			break;
		default:
			assert(0);
	}
}


void
notif_chain_invoke(notif_chain_t *notif_chain,
		notif_chain_elem_t *notif_chain_elem){

	notif_chain_elem_t *notif_chain_elem_curr;

	ITERTAE_NOTIF_CHAIN_BEGIN(notif_chain, notif_chain_elem_curr){

		if(notif_chain->comp_cb &&
				notif_chain_elem &&
				notif_chain_elem->data.app_key_data &&
				notif_chain_elem->data.app_key_data_size &&
				notif_chain_elem_curr->data.app_key_data &&
				notif_chain_elem_curr->data.app_key_data_size &&
				notif_chain->comp_cb(notif_chain_elem_curr->data.app_key_data,
					notif_chain_elem_curr->data.app_key_data_size,
					notif_chain_elem->data.app_key_data,
					notif_chain_elem->data.app_key_data_size) != 0){

			continue;
		}

		notif_chain_elem_curr->notif_code = NOTIF_C_UNKNOWN;

		if(notif_chain_elem){
			notif_chain_elem_curr->notif_code = 
				notif_chain_elem->notif_code;
			notif_chain_elem_curr->data.is_alloc_app_data_to_notify = 
				notif_chain_elem->data.is_alloc_app_data_to_notify;
			notif_chain_elem_curr->data.app_data_to_notify = 
				notif_chain_elem->data.app_data_to_notify;
			notif_chain_elem_curr->data.app_data_to_notify_size =
				notif_chain_elem->data.app_data_to_notify_size;
		}
		notif_chain_invoke_communication_channel(
				notif_chain,
				notif_chain_elem_curr);

	} ITERTAE_NOTIF_CHAIN_END(notif_chain, notif_chain_elem_curr);
}

void
notif_chain_dump(notif_chain_t *notif_chain){

	char buffer[256];

	notif_chain_elem_t *notif_chain_elem_curr;

	printf("notif chain Name : %s\n", notif_chain->name);

	ITERTAE_NOTIF_CHAIN_BEGIN(notif_chain, notif_chain_elem_curr){

		printf("\tprev %p next %p client_id %u app_key_data_ptr %p "
				"app_key_data_size %u app_cb %p\n",
				notif_chain_elem_curr->prev,
				notif_chain_elem_curr->next,
				notif_chain_elem_curr->client_id,
				notif_chain_elem_curr->data.app_key_data,
				notif_chain_elem_curr->data.app_key_data_size,
				NOTIF_CHAIN_ELEM_APP_CB(
					(&notif_chain_elem_curr->notif_chain_comm_channel)));

		if(notif_chain->print_cb){
			notif_chain->print_cb(
					notif_chain_elem_curr->data.app_key_data,
					notif_chain_elem_curr->data.app_key_data_size,
					buffer, sizeof(buffer));
			printf("\tApp Key Data : \n");
			printf("%s\n", buffer);
		}

	} ITERTAE_NOTIF_CHAIN_END(notif_chain, notif_chain_elem_curr);
}

bool
notif_chain_subscribe(char *notif_chain_name,
		notif_chain_elem_t *notif_chain_elem,
		bool request_malloc){

	notif_chain_t *notif_chain;

	notif_ch_type_t notif_ch_type = NOTIF_CHAIN_ELEM_TYPE(notif_chain_elem);

	notif_chain = notif_chain_lookup_notif_chain_by_name(notif_chain_name);
	if(!notif_chain){
		printf("Appln dont have Notif Chain with name %s\n", 
				notif_chain_name);
		return false;
	}

	switch(notif_ch_type){

		case NOTIF_C_CALLBACKS:
		case NOTIF_C_MSG_Q:
		case NOTIF_C_AF_UNIX:
		case NOTIF_C_INET_SOCKETS:
			notif_chain_register_chain_element(notif_chain, notif_chain_elem, request_malloc);
			break;
		case NOTIF_C_ANY:
		case NOTIF_C_NOT_KNOWN:
			return false;
		default:
			return false;
	}
	return true;
}


bool
notif_chain_unsubscribe(char *notif_chain_name,
		notif_chain_elem_t *notif_chain_elem){

	notif_chain_t *notif_chain;

	notif_ch_type_t notif_ch_type = NOTIF_CHAIN_ELEM_TYPE(notif_chain_elem);

	notif_chain = notif_chain_lookup_notif_chain_by_name(notif_chain_name);

	if(!notif_chain){
		printf("Appln dont have Notif Chain with name %s\n", 
				notif_chain_name);
		return false;
	}

	switch(notif_ch_type){

		case NOTIF_C_CALLBACKS:
		case NOTIF_C_MSG_Q:
		case NOTIF_C_AF_UNIX:
		case NOTIF_C_INET_SOCKETS:
			notif_chain_deregister_chain_element(notif_chain, notif_chain_elem);
			break;
		case NOTIF_C_ANY:
		case NOTIF_C_NOT_KNOWN:
			return false;
		default:
			return false;
	}
	return true;
}

void
notif_chain_elem_remove(notif_chain_t *notif_chain,
		notif_chain_elem_t *notif_chain_elem){

	if(!notif_chain_elem->prev){
		if(notif_chain_elem->next){
			notif_chain_elem->next->prev = NULL;
			notif_chain->head = notif_chain_elem->next;
			notif_chain_elem->next = 0;
			return;
		}
		return;
	}
	if(!notif_chain_elem->next){
		notif_chain_elem->prev->next = NULL;
		notif_chain_elem->prev = NULL;
		return;
	}

	notif_chain_elem->prev->next = notif_chain_elem->next;
	notif_chain_elem->next->prev = notif_chain_elem->prev;
	notif_chain_elem->prev = 0;
	notif_chain_elem->next = 0;
}

/* Two notif_chain_elem_t objects are said to be equal if :
 * 1. client ID is same and
 * 2. key is same
 * */
bool
notif_chain_is_duplicate_notif_chain_element(
                notif_chain_elem_t *notif_chain_elem,
				uint32_t client_id,
                void *app_key_data,
				uint32_t app_key_data_size){

	if (notif_chain_elem->client_id != client_id){

		return false;
	}

	if (notif_chain_elem->data.app_key_data_size !=
			app_key_data_size){

		return false;
	}

	return (memcmp(notif_chain_elem->data.app_key_data,
			app_key_data, app_key_data_size) == 0);
}

notif_chain_elem_t *
notif_chain_lookup_notif_chain_element(
                notif_chain_t *notif_chain,
                uint32_t client_id,
                void *app_key_data,
                uint32_t app_key_data_size){

	notif_chain_elem_t *notif_chain_elem;
	
	ITERTAE_NOTIF_CHAIN_BEGIN(notif_chain, notif_chain_elem){

		if (notif_chain_is_duplicate_notif_chain_element(
										notif_chain_elem,
										client_id,
										app_key_data,
										app_key_data_size)) {
			return notif_chain_elem;
		}			
	} ITERTAE_NOTIF_CHAIN_END(notif_chain, notif_chain_elem);		
	return NULL;
}

bool
notif_chain_process_remote_subscriber_request(
		char *subs_tlv_buffer, 
		uint32_t subs_tlv_buffer_size){

	notif_ch_type_t notif_ch_type;
	notif_ch_notify_opcode_t notif_code;
	notif_chain_elem_t *notif_chain_elem;
	char notif_chain_name[NOTIF_C_NOTIF_CHAIN_NAME_VALUE_LEN];

	notif_chain_elem = notif_chain_deserialize_notif_chain_elem(
			subs_tlv_buffer,
			subs_tlv_buffer_size,
			notif_chain_name);

	notif_ch_type = NOTIF_CHAIN_ELEM_TYPE(notif_chain_elem);

	assert(notif_ch_type != NOTIF_C_CALLBACKS);

	notif_code = notif_chain_elem->notif_code;

	switch(notif_code){

		case PUB_TO_SUBS_NOTIF_C_CREATE:
		case PUB_TO_SUBS_NOTIF_C_UPDATE:
		case PUB_TO_SUBS_NOTIF_C_DELETE:
			assert(0);
		case SUBS_TO_PUB_NOTIF_C_SUBSCRIBE:
			notif_chain_subscribe(notif_chain_name,
					notif_chain_elem, false);
			break;
		case SUBS_TO_PUB_NOTIF_C_UNSUBSCRIBE:
			notif_chain_unsubscribe(notif_chain_name,
					notif_chain_elem);
			break;
		case SUBS_TO_PUB_NOTIFY_C_NOTIFY_ALL:
			break;
		case SUBS_TO_PUB_NOTIFY_C_CLIENT_UNSUBSCRIBE_ALL:
			break;
		case NOTIF_C_UNKNOWN:
		default:
			return false;
	}
	return true;
}

/* APIs to be used by client/subscribers to subscribe
 * with publishers*/
bool
notif_chain_subscribe_by_callback(
		char *notif_chain_name,
		void *key,
		uint32_t key_size,
		uint32_t client_id,
		notif_chain_app_cb cb){

	notif_chain_elem_t notif_chain_elem;
	memset(&notif_chain_elem, 0, sizeof(notif_chain_elem_t));
	notif_chain_elem.client_id = client_id;
	notif_chain_elem.notif_code = SUBS_TO_PUB_NOTIF_C_SUBSCRIBE;
	notif_chain_elem.data.app_key_data = key;
	notif_chain_elem.data.app_key_data_size = key_size;
	notif_chain_comm_channel_t *
		notif_chain_comm_channel = &notif_chain_elem.notif_chain_comm_channel;
	notif_chain_comm_channel->notif_ch_type = NOTIF_C_CALLBACKS;
	NOTIF_CHAIN_ELEM_APP_CB(notif_chain_comm_channel) = cb;
	return notif_chain_subscribe(notif_chain_name, &notif_chain_elem, true);
}

bool
notif_chain_subscribe_by_inet_skt(
		char *notif_chain_name,
		void *key,
		uint32_t key_size,
		uint32_t client_id,
		char *subs_addr,
		uint16_t subs_port_no,
		uint16_t protocol_no,
		char *publisher_addr,
		uint16_t publisher_port_no){


	char *subs_tlv_buff = NULL;
	uint32_t subs_tlv_buff_size;
	notif_chain_elem_t notif_chain_elem;

	memset(&notif_chain_elem, 0, sizeof(notif_chain_elem_t));
	notif_chain_elem.client_id = client_id;
	notif_chain_elem.notif_code = SUBS_TO_PUB_NOTIF_C_SUBSCRIBE;

	notif_chain_comm_channel_t *notif_chain_comm_channel = 
		&notif_chain_elem.notif_chain_comm_channel;

	notif_chain_comm_channel->notif_ch_type = NOTIF_C_INET_SOCKETS;

	NOTIF_CHAIN_ELEM_IP_ADDR(notif_chain_comm_channel) = 
		tcp_ip_covert_ip_p_to_n(subs_addr);

	NOTIF_CHAIN_ELEM_PORT_NO(notif_chain_comm_channel) = subs_port_no;
	NOTIF_CHAIN_ELEM_PROTO(notif_chain_comm_channel) = protocol_no;

	subs_tlv_buff_size = notif_chain_serialize_notif_chain_elem(
			notif_chain_name,
			&notif_chain_elem,
			0,
			0,
			&subs_tlv_buff);

	if(!subs_tlv_buff_size){
		printf("%s() : Error : Msg send to publisher failed\n",
				__FUNCTION__);
		return false;
	}

	notif_chain_send_msg_to_publisher(publisher_addr, 
			publisher_port_no,
			subs_tlv_buff,
			subs_tlv_buff_size);

	notif_chain_free_notif_chain_elem_internals(
			&notif_chain_elem);

	free(subs_tlv_buff);
	subs_tlv_buff = NULL;
	return true;
}

bool
notif_chain_subscribe_by_unix_skt(
		char *notif_chain_name,
		void *key,
		uint32_t key_size,
		uint32_t client_id,
		char *subs_unix_skt_name,
		char *publisher_addr,
		uint16_t publisher_port_no){

	char *subs_tlv_buff = NULL;
	uint32_t subs_tlv_buff_size;
	notif_chain_elem_t notif_chain_elem;

	memset(&notif_chain_elem, 0, sizeof(notif_chain_elem_t));
	notif_chain_elem.client_id = client_id;
	notif_chain_elem.notif_code = SUBS_TO_PUB_NOTIF_C_SUBSCRIBE;

	notif_chain_comm_channel_t *notif_chain_comm_channel = 
		&notif_chain_elem.notif_chain_comm_channel;

	notif_chain_comm_channel->notif_ch_type = NOTIF_C_AF_UNIX;

	strncpy(NOTIF_CHAIN_ELEM_SKT_NAME(notif_chain_comm_channel),
			subs_unix_skt_name, 
			NOTIF_C_COMM_CHANNEL_UNIX_SKT_NAME_VALUE_LEN);

	subs_tlv_buff_size = notif_chain_serialize_notif_chain_elem(
			notif_chain_name,
			&notif_chain_elem,
			0,
			0,
			&subs_tlv_buff);

	if(!subs_tlv_buff_size){
		printf("%s() : Error : Msg send to publisher failed\n",
				__FUNCTION__);
		return false;
	}

	notif_chain_send_msg_to_publisher(publisher_addr, 
			publisher_port_no,
			subs_tlv_buff,
			subs_tlv_buff_size);

	notif_chain_free_notif_chain_elem_internals(
			&notif_chain_elem);

	free(subs_tlv_buff);
	subs_tlv_buff = NULL;
	return true;
}

bool
notif_chain_subscribe_msgq(
		char *notif_chain_name,
		void *key,
		uint32_t key_size,
		uint32_t client_id,
		char *subs_msgq_name,
		char *publisher_addr,
		uint16_t publisher_port_no){

	char *subs_tlv_buff = NULL;
	uint32_t subs_tlv_buff_size;
	notif_chain_elem_t notif_chain_elem;

	memset(&notif_chain_elem, 0, sizeof(notif_chain_elem_t));
	notif_chain_elem.client_id = client_id;
	notif_chain_elem.notif_code = SUBS_TO_PUB_NOTIF_C_SUBSCRIBE;

	notif_chain_elem.data.app_key_data = calloc(1, key_size);

	memcpy((char *)notif_chain_elem.data.app_key_data,
		   (char *)key, key_size);
	notif_chain_elem.data.app_key_data_size = key_size;

	notif_chain_comm_channel_t *notif_chain_comm_channel = 
		&notif_chain_elem.notif_chain_comm_channel;

	notif_chain_comm_channel->notif_ch_type = NOTIF_C_MSG_Q;

	strncpy(NOTIF_CHAIN_ELEM_MSGQ_NAME(notif_chain_comm_channel),
			subs_msgq_name,
			NOTIF_C_COMM_CHANNEL_MSGQ_NAME_VALUE_LEN);

	subs_tlv_buff_size = notif_chain_serialize_notif_chain_elem(
			notif_chain_name,
			&notif_chain_elem,
			0,
			0,
			&subs_tlv_buff);

	if(!subs_tlv_buff_size){
		printf("%s() : Error : Msg send to publisher failed\n",
				__FUNCTION__);
		return false;
	}

	notif_chain_send_msg_to_publisher(publisher_addr, 
			publisher_port_no,
			subs_tlv_buff,
			subs_tlv_buff_size);

	notif_chain_free_notif_chain_elem_internals(
			&notif_chain_elem);

	free(subs_tlv_buff);
	subs_tlv_buff = NULL;
	return true;
}

/* APIs for Rx/Tx Msgs between Publisher and Subscribers
 * Over Network UDP Sockets*/
static int
notif_chain_send_udp_msg(char *dest_ip_addr,
		uint16_t dest_port_no,
		char *msg,
		uint32_t msg_size){

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

int
notif_chain_send_msg_to_publisher(char *publisher_addr,
		uint16_t publisher_port_no,
		char *msg,
		uint32_t msg_size){

	return notif_chain_send_udp_msg(publisher_addr, 
			publisher_port_no,
			msg,
			msg_size);
}

int
notif_chain_send_msg_to_subscriber(char *subscriber_addr,
		uint16_t subscriber_port_no,
		char *msg,
		uint32_t msg_size){

	return notif_chain_send_udp_msg(subscriber_addr,
			subscriber_port_no,
			msg,
			msg_size);
}

uint32_t
notif_chain_compute_required_tlv_buffer_size_for_notif_chain_elem_encoding(
		notif_chain_elem_t *notif_chain_elem){

	uint32_t size = 0;

	/* Mandatory TLVs*/
	/* Notif chain name*/
	size += TLV_OVERHEAD_SIZE + NOTIF_C_NOTIF_CHAIN_NAME_VALUE_LEN;

	/* client id*/
	size += TLV_OVERHEAD_SIZE + NOTIF_C_CLIENT_ID_VALUE_LEN;

	/*notif code*/
	size += TLV_OVERHEAD_SIZE + NOTIF_C_NOTIF_CODE_VALUE_LEN;

	/* Non-Mandatory TLV*/
	switch(NOTIF_CHAIN_ELEM_TYPE(notif_chain_elem)){

		case NOTIF_C_CALLBACKS:
			break;
		case NOTIF_C_MSG_Q:
			size += TLV_OVERHEAD_SIZE + NOTIF_C_COMM_CHANNEL_TYPE_VALUE_LEN;
			size += TLV_OVERHEAD_SIZE + NOTIF_C_COMM_CHANNEL_MSGQ_NAME_VALUE_LEN;
			break;
		case NOTIF_C_AF_UNIX:
			size += TLV_OVERHEAD_SIZE + NOTIF_C_COMM_CHANNEL_TYPE_VALUE_LEN;
			size += TLV_OVERHEAD_SIZE + NOTIF_C_COMM_CHANNEL_UNIX_SKT_NAME_VALUE_LEN;
			break;
		case NOTIF_C_INET_SOCKETS:
			size += TLV_OVERHEAD_SIZE + NOTIF_C_COMM_CHANNEL_TYPE_VALUE_LEN;
			size += TLV_OVERHEAD_SIZE + NOTIF_C_IP_ADDR_VALUE_LEN;
			size += TLV_OVERHEAD_SIZE + NOTIF_C_PORT_NO_VALUE_LEN;
			size += TLV_OVERHEAD_SIZE + NOTIF_C_PROTOCOL_NO_VALUE_LEN;
			break;
		case NOTIF_C_ANY:
			/* Encode NOTIF_C_ANY as it is. Useful for publisher to choose 
			 * send notif to subscriber without considering the channel type
			 * as filter criteria*/
			size += TLV_OVERHEAD_SIZE + NOTIF_C_COMM_CHANNEL_TYPE_VALUE_LEN;
			break;
		case NOTIF_C_NOT_KNOWN:
			break;
		default:
			;
	}


	if(notif_chain_elem->data.app_key_data && 
			notif_chain_elem->data.app_key_data_size){

		size += TLV_OVERHEAD_SIZE + notif_chain_elem->data.app_key_data_size;
	}

	if(notif_chain_elem->data.app_data_to_notify &&
			notif_chain_elem->data.app_data_to_notify_size){

		size += TLV_OVERHEAD_SIZE + notif_chain_elem->data.app_data_to_notify_size;
	}

	return size;
}

uint32_t
notif_chain_serialize_notif_chain_elem(
		char *notif_chain_name,
		notif_chain_elem_t *notif_chain_elem,
		char *output_buffer_provided,
		uint32_t output_buffer_provided_size,
		char **output_buffer_computed){

	char *output_buff;
	uint32_t tlv_buff_cal_size = 0 ;
	notif_chain_comm_channel_t *
		notif_chain_comm_channel = NULL;

	/* If neither output buffer provided, nor are
	 * we being asked to allocate new one, then
	 * what is the purpose of my life*/
	if(!output_buffer_provided && 
			!output_buffer_computed){
		assert(0);
	}

	/* If i am asked to compute the output buffer,
	 * then i must not be supplied with buffer pointer.
	 * I will allocate one */
	if(output_buffer_computed && 
			*output_buffer_computed != NULL){    
		assert(0);
	}

	tlv_buff_cal_size = 
		notif_chain_compute_required_tlv_buffer_size_for_notif_chain_elem_encoding(
				notif_chain_elem);

	if(output_buffer_provided && 
			(tlv_buff_cal_size > output_buffer_provided_size)){

		printf("%s() : Error : Not big enough output buffer provided\n",
				__FUNCTION__);

		return 0;
	}

	output_buff = NULL;

	if(output_buffer_provided){

		output_buff = output_buffer_provided;
	}
	else if(output_buffer_computed){

		output_buff = (char *)calloc(1, tlv_buff_cal_size);
		*output_buffer_computed = output_buff;
	}

	if(!output_buff){
		return 0;
	}

	/* Mandatory TLVs First*/
	output_buff = tlv_buffer_insert_tlv(output_buff,
			NOTIF_C_NOTIF_CHAIN_NAME_TLV,
			NOTIF_C_NOTIF_CHAIN_NAME_VALUE_LEN,
			notif_chain_name);

	output_buff = tlv_buffer_insert_tlv(output_buff,
			NOTIF_C_CLIENT_ID_TLV,
			NOTIF_C_CLIENT_ID_VALUE_LEN,
			(char *)&(notif_chain_elem->client_id));

	output_buff = tlv_buffer_insert_tlv(output_buff,
			NOTIF_C_NOTIF_CODE_TLV,
			NOTIF_C_NOTIF_CODE_VALUE_LEN,
			(char *)&(notif_chain_elem->notif_code));

	notif_chain_comm_channel = &notif_chain_elem->notif_chain_comm_channel;

	/*Non-Mandatory TLVs now*/
	switch(NOTIF_CHAIN_ELEM_TYPE(notif_chain_elem)){
		case NOTIF_C_CALLBACKS:
			break;
		case NOTIF_C_MSG_Q:
		case NOTIF_C_AF_UNIX:
			output_buff = tlv_buffer_insert_tlv(output_buff,
					NOTIF_C_COMM_CHANNEL_TYPE_TLV,
					NOTIF_C_COMM_CHANNEL_TYPE_VALUE_LEN,
					(char *)&(NOTIF_CHAIN_ELEM_TYPE(notif_chain_elem)));
			output_buff = tlv_buffer_insert_tlv(output_buff,
					NOTIF_C_COMM_CHANNEL_NAME_TLV,

					NOTIF_CHAIN_ELEM_TYPE(notif_chain_elem) == NOTIF_C_MSG_Q ?
					NOTIF_C_COMM_CHANNEL_MSGQ_NAME_VALUE_LEN :
					NOTIF_C_COMM_CHANNEL_UNIX_SKT_NAME_VALUE_LEN,

					NOTIF_CHAIN_ELEM_TYPE(notif_chain_elem) == NOTIF_C_MSG_Q ?
					NOTIF_CHAIN_ELEM_MSGQ_NAME(notif_chain_comm_channel) :
					NOTIF_CHAIN_ELEM_SKT_NAME(notif_chain_comm_channel));
			break;
		case NOTIF_C_INET_SOCKETS:
			output_buff = tlv_buffer_insert_tlv(output_buff,
					NOTIF_C_COMM_CHANNEL_TYPE_TLV,
					NOTIF_C_COMM_CHANNEL_TYPE_VALUE_LEN,
					(char *)&(NOTIF_CHAIN_ELEM_TYPE(notif_chain_elem)));

			output_buff = tlv_buffer_insert_tlv(output_buff,
					NOTIF_C_IP_ADDR_TLV,
					NOTIF_C_IP_ADDR_VALUE_LEN,
					tcp_ip_covert_ip_n_to_p(
						NOTIF_CHAIN_ELEM_IP_ADDR(notif_chain_comm_channel), 0));

			output_buff = tlv_buffer_insert_tlv(output_buff,
					NOTIF_C_PORT_NO_TLV,
					NOTIF_C_PORT_NO_VALUE_LEN,
					(char *)&(NOTIF_CHAIN_ELEM_PORT_NO(notif_chain_comm_channel)));

			output_buff = tlv_buffer_insert_tlv(output_buff,
					NOTIF_C_PROTOCOL_NO_TLV,
					NOTIF_C_PROTOCOL_NO_VALUE_LEN,
					(char *)&(NOTIF_CHAIN_ELEM_PROTO(notif_chain_comm_channel)));
			break;
		case NOTIF_C_ANY:
			output_buff = tlv_buffer_insert_tlv(output_buff,
					NOTIF_C_COMM_CHANNEL_TYPE_TLV,
					NOTIF_C_COMM_CHANNEL_TYPE_VALUE_LEN,
					(char *)&(NOTIF_CHAIN_ELEM_TYPE(notif_chain_elem))); 
		case NOTIF_C_NOT_KNOWN:
			break;
		default:
			;
	}


	if(notif_chain_elem->data.app_key_data &&
			notif_chain_elem->data.app_key_data_size){

		output_buff = tlv_buffer_insert_tlv(output_buff,
				NOTIF_C_APP_KEY_DATA_TLV,
				notif_chain_elem->data.app_key_data_size,
				(char *)notif_chain_elem->data.app_key_data);
	}

	if(notif_chain_elem->data.app_data_to_notify &&
			notif_chain_elem->data.app_data_to_notify_size){

		output_buff = tlv_buffer_insert_tlv(output_buff,
				NOTIF_C_APP_DATA_TO_NOTIFY_TLV,
				notif_chain_elem->data.app_data_to_notify_size,
				(char *)notif_chain_elem->data.app_data_to_notify);
	}

	return tlv_buff_cal_size;
}

notif_chain_elem_t *
notif_chain_deserialize_notif_chain_elem(
		char *tlv_buffer,
		uint32_t tlv_buff_size,
		char *notif_chain_name /*o/p*/){

	notif_chain_elem_t *notif_chain_elem;
	uint8_t tlv_type, tlv_len, *tlv_value;
	notif_chain_comm_channel_t *
		notif_chain_comm_channel = NULL;

	assert(notif_chain_name);

	notif_chain_elem = calloc(1, sizeof(notif_chain_elem_t));
	notif_chain_comm_channel = &notif_chain_elem->notif_chain_comm_channel;

	ITERATE_TLV_BEGIN(tlv_buffer, tlv_type,
			tlv_len, tlv_value, 
			tlv_buff_size){

		switch(tlv_type){

			case NOTIF_C_NOTIF_CHAIN_NAME_TLV:
				strncpy(notif_chain_name, tlv_value, tlv_len);
				break;
			case NOTIF_C_CLIENT_ID_TLV:
				memcpy((char *)&notif_chain_elem->client_id,
						tlv_value, tlv_len);
				break;
			case NOTIF_C_COMM_CHANNEL_TYPE_TLV:
				memcpy((char *)&(NOTIF_CHAIN_ELEM_TYPE(notif_chain_elem)),
						tlv_value, tlv_len);
				break;
			case NOTIF_C_COMM_CHANNEL_NAME_TLV:
				switch(NOTIF_CHAIN_ELEM_TYPE(notif_chain_elem)){
					case NOTIF_C_ANY:
						break;
					case NOTIF_C_CALLBACKS:
						break;
					case NOTIF_C_MSG_Q:
						memcpy(NOTIF_CHAIN_ELEM_MSGQ_NAME(notif_chain_comm_channel),
								tlv_value, tlv_len);
						break;
					case NOTIF_C_AF_UNIX:
						memcpy(NOTIF_CHAIN_ELEM_SKT_NAME(notif_chain_comm_channel),
								tlv_value, tlv_len);
						break;
					case NOTIF_C_INET_SOCKETS:
						break;
					case NOTIF_C_NOT_KNOWN:
						break;
					default:
						;
				}
				break;
			case NOTIF_C_IP_ADDR_TLV:
				memcpy((char *)&(NOTIF_CHAIN_ELEM_IP_ADDR(notif_chain_comm_channel)),
						tlv_value, tlv_len);
				break;
			case NOTIF_C_PORT_NO_TLV:
				memcpy((char *)&(NOTIF_CHAIN_ELEM_PORT_NO(notif_chain_comm_channel)),
						tlv_value, tlv_len);
				break;
			case NOTIF_C_NOTIF_CODE_TLV:
				memcpy((char *)&notif_chain_elem->notif_code,
						tlv_value, tlv_len);
				break;
			case NOTIF_C_PROTOCOL_NO_TLV:
				memcpy((char *)&(NOTIF_CHAIN_ELEM_PROTO(notif_chain_comm_channel)),
						tlv_value, tlv_len);
				break;
			case NOTIF_C_APP_KEY_DATA_TLV:
				notif_chain_elem->data.app_key_data = (char *)calloc(1,
						tlv_len);
				memcpy((char *)notif_chain_elem->data.app_key_data,
						tlv_value, tlv_len);
				notif_chain_elem->data.app_key_data_size = (uint32_t)tlv_len;
				break;
			case NOTIF_C_APP_DATA_TO_NOTIFY_TLV:
				notif_chain_elem->data.app_data_to_notify = (char *)calloc(1,
						tlv_len);
				memcpy((char *)notif_chain_elem->data.app_data_to_notify,
						tlv_value, tlv_len);
				notif_chain_elem->data.app_data_to_notify_size = (uint32_t)tlv_len;
				notif_chain_elem->data.is_alloc_app_data_to_notify = true;
				break;
			default:
				;
		}
	}ITERATE_TLV_END(tlv_buffer, tlv_type,
			tlv_len, tlv_value,
			tlv_buff_size);

	return notif_chain_elem;
}   

