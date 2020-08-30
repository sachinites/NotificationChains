/*
 * =====================================================================================
 *
 *       Filename:  rt.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  02/22/2020 06:14:33 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Er. Abhishek Sagar, Juniper Networks (https://csepracticals.wixsite.com/csepracticals), sachinites@gmail.com
 *        Company:  Juniper Networks
 *
 *        This file is part of the Netlink Sockets distribution (https://github.com/sachinites) 
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

#ifndef __RT__
#define __RT__

#include <stdbool.h>

typedef struct rt_entry_{

    char dest_ip[16];
    char mask;
    char gw_ip[16];
    char oif[32];
    struct rt_entry_ *prev;
    struct rt_entry_ *next;
} rt_entry_t;

typedef struct rt_table_{

    rt_entry_t *head;
} rt_table_t;

void
rt_init_rt_table(rt_table_t *rt_table);

bool
rt_add_new_rt_entry(rt_table_t *rt_table,
    char *dest_ip, char mask, char *gw_ip, char *oif);

bool
rt_delete_rt_entry(rt_table_t *rt_table,
    char *dest_ip, char mask);

bool
rt_update_rt_entry(rt_table_t *rt_table,
    char *dest_ip, char mask, 
    char *new_gw_ip, char *new_oif);

void
rt_clear_rt_table(rt_table_t *rt_table);

void
rt_free_rt_table(rt_table_t *rt_table);

void
rt_dump_rt_table(rt_table_t *rt_table);

static inline void
rt_entry_remove(rt_table_t *rt_table,
                rt_entry_t *rt_entry){

    if(!rt_entry->prev){
        if(rt_entry->next){
            rt_entry->next->prev = NULL;
            rt_table->head = rt_entry->next;
            rt_entry->next = 0;
            return;
        }
        return;
    }
    if(!rt_entry->next){
        rt_entry->prev->next = NULL;
        rt_entry->prev = NULL;
        return;
    }

    rt_entry->prev->next = rt_entry->next;
    rt_entry->next->prev = rt_entry->prev;
    rt_entry->prev = 0;
    rt_entry->next = 0;
}

#define ITERTAE_RT_TABLE_BEGIN(rt_table_ptr, rt_entry_ptr)                \
{                                                                         \
    rt_entry_t *_next_rt_entry;                                           \
    for((rt_entry_ptr) = (rt_table_ptr)->head;                            \
            (rt_entry_ptr);                                               \
            (rt_entry_ptr) = _next_rt_entry) {                            \
        _next_rt_entry = (rt_entry_ptr)->next;

#define ITERTAE_RT_TABLE_END(rt_table_ptr, rt_entry_ptr)  }}

#endif /* __RT__ */
