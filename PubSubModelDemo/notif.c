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
		app_key_data_print_cb print_cb) {

	memset(notif_chain, 0, sizeof(notif_chain_t));
	strncpy(notif_chain->name, chain_name, sizeof(notif_chain->name));
	notif_chain->name[sizeof(notif_chain->name) -1] = '\0';
	notif_chain->comp_cb = comp_cb;
	notif_chain->print_cb = print_cb;
}

void
notif_chain_release_communication_channel_resources(
		notif_chain_comm_channel_t *channel){

	notif_ch_type_t notif_ch_type;

	notif_ch_type = channel->notif_ch_type;

	switch(notif_ch_type){

		case NOTIF_C_CALLBACKS:
			return;
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
		notif_chain_elem_t *notif_chain_elem){

	notif_chain_elem_t *new_notif_chain_elem = 
		calloc(1,sizeof(notif_chain_elem_t));

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

		case NOTIF_C_CALLBACKS:
			if(NOTIF_CHAIN_ELEM_APP_CB(channel1) != 
					NOTIF_CHAIN_ELEM_APP_CB(channel2))
				return false;
			return true;
		case NOTIF_C_NOT_KNOWN:
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
		case NOTIF_C_NOT_KNOWN:
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
		notif_chain_elem_t *notif_chain_elem){

	notif_chain_t *notif_chain;

	notif_ch_type_t notif_ch_type = NOTIF_CHAIN_ELEM_TYPE(notif_chain_elem);

	notif_chain = (notif_chain_lookup_notif_chain_by_name)(notif_chain_name);
	if(!notif_chain){
		printf("Appln dont have Notif Chain with name %s\n", 
				notif_chain_name);
		return false;
	}

	switch(notif_ch_type){

		case NOTIF_C_CALLBACKS:
			notif_chain_register_chain_element(notif_chain, notif_chain_elem);
			break;
		case NOTIF_C_NOT_KNOWN:
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
			notif_chain_deregister_chain_element(notif_chain, notif_chain_elem);
			break;
		case NOTIF_C_NOT_KNOWN:
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
	return notif_chain_subscribe(notif_chain_name, &notif_chain_elem);
}

