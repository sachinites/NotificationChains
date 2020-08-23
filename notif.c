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

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include "notif.h"

void
notif_chain_init(notif_chain_t *notif_chain,
                char *chain_name,
                notif_chain_comp_fn comp_fn){

    memset(notif_chain, 0, sizeof(notif_chain_t));
    strncpy(notif_chain->name, chain_name, sizeof(notif_chain->name));
    notif_chain->name[sizeof(notif_chain->name) -1] = '\0';
    notif_chain->comp_fn = comp_fn;
}

void
notif_chain_delete(notif_chain_t *notif_chain){

    notif_chain_elem_t *notif_chain_elem;
    
    ITERTAE_NOTIF_CHAIN_BEGIN(notif_chain, notif_chain_elem){

        notif_chain_elem_remove(notif_chain, notif_chain_elem);
        free(notif_chain_elem);
    } ITERTAE_NOTIF_CHAIN_END(notif_chain, notif_chain_elem);
    notif_chain->head = NULL;
}

bool
notif_chain_register_chain_element(notif_chain_t *notif_chain,
                notif_chain_elem_t *notif_chain_elem){

    notif_chain_elem_t *head = notif_chain->head;
    notif_chain->head = notif_chain_elem;
    notif_chain_elem->prev = 0;
    notif_chain_elem->next = head;
    head->prev = notif_chain_elem;
}

bool
notif_chain_deregister_chain_element(notif_chain_t *notif_chain,
                notif_chain_elem_t *notif_chain_elem){

    bool is_removed = false;
    notif_chain_elem_t *notif_chain_elem_curr;
    
    ITERTAE_NOTIF_CHAIN_BEGIN(notif_chain, notif_chain_elem_curr){

        if(notif_chain->comp_fn(notif_chain_elem_curr->data,
            notif_chain_elem_curr->data_size, 
            notif_chain_elem->data, 
            notif_chain_elem->data_size) == 0){
            notif_chain_elem_remove(notif_chain, notif_chain_elem_curr);
            free(notif_chain_elem_curr);
            is_removed = true;
        }
    } ITERTAE_NOTIF_CHAIN_END(notif_chain, notif_chain_elem_curr);
    return is_removed;
}

bool
notif_chain_invoke(notif_chain_t *notif_chain,
                notif_chain_elem_t *notif_chain_elem){

    notif_chain_elem_t *notif_chain_elem_curr;
    
    ITERTAE_NOTIF_CHAIN_BEGIN(notif_chain, notif_chain_elem_curr){

        if(notif_chain->comp_fn(notif_chain_elem_curr->data,
            notif_chain_elem_curr->data_size, 
            notif_chain_elem->data,
            notif_chain_elem->data_size) == 0){

            notif_chain_elem_curr->app_cb(
                notif_chain_elem->data, notif_chain_elem->data_size);
        }
    } ITERTAE_NOTIF_CHAIN_END(notif_chain, notif_chain_elem_curr);
}

void
notif_chain_dump(notif_chain_t *notif_chain){

    notif_chain_elem_t *notif_chain_elem_curr;

    printf("notif chain Name : %s\n", notif_chain->name);

    ITERTAE_NOTIF_CHAIN_BEGIN(notif_chain, notif_chain_elem_curr){

        printf("\tprev %p next %p data %p data_size %u app_cb %p\n",
            notif_chain_elem_curr->prev,
            notif_chain_elem_curr->next,
            notif_chain_elem_curr->data,
            notif_chain_elem_curr->data_size,
            notif_chain_elem_curr->app_cb);
    } ITERTAE_NOTIF_CHAIN_END(notif_chain, notif_chain_elem_curr);
}

