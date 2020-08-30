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
#include "notif.h"
#include "rt.h"
#include <sys/types.h> /*for pid_t*/
#include <unistd.h>    /*for getpid()*/

static notif_chain_t notif_chain;

void 
main_menu(rt_table_t *rt){

    int choice;

    /*Add some default entries in routing table*/
    rt_add_new_rt_entry(rt, "122.1.1.1", 32, "10.1.1.1", "eth0");
    rt_add_new_rt_entry(rt, "122.1.1.2", 32, "10.1.1.2", "eth1");
    rt_add_new_rt_entry(rt, "122.1.1.3", 32, "10.1.1.3", "eth2");
    rt_add_new_rt_entry(rt, "122.1.1.4", 32, "10.1.1.4", "eth3");
    rt_add_new_rt_entry(rt, "122.1.1.5", 32, "10.1.1.5", "eth4");

    while(1){
        printf("1. Add rt table entry\n");
        printf("2. Update rt table entry\n");
        printf("3. Delete rt table entry\n");
        printf("4. Dump rt table\n");
        printf("Enter Choice :");
        choice = 0;
        scanf("%d", &choice);
        switch(choice){
            case 1:
                {
                    char dest[16];
                    char mask;
                    char oif[32];
                    char gw[16];
                    printf("Enter Destination :");
                    scanf("%s", dest);
                    printf("Mask : ");
                    scanf("%d", &mask);
                    printf("Enter oif name :");
                    scanf("%s", oif);
                    printf("Enter Gateway IP :");
                    scanf("%s", gw);
                    if(!rt_add_new_rt_entry(rt, dest, mask,
                        gw, oif)){
                        printf("Error : Could not add an entry\n");
                    }
                    /*Invoke Notif Chain*/
                    notif_chain_invoke(&notif_chain, 0);
                }  
            break;
            case 2:
            case 3:
            case 4:
                rt_dump_rt_table(rt);
                break;
            default:
                exit(0);
        }
    }
}

static notif_chain_t *
get_notif_chain(char *notif_chain_name){

    return &notif_chain;    
}

static void
test_cb(void *data, uint32_t data_size){

    printf("%s() invoked\n", __FUNCTION__);
}

int
main(int argc, char **argv){

    rt_table_t rt;
    rt_init_rt_table(&rt);

    /*Create a notif Chain for the publisher*/
    notif_chain_init(&notif_chain,
        "rt table update",
        0, 0, 
        get_notif_chain);

    /*Do registration with Notification Chain.
     * This code acts as a client/subscriber code*/
    notif_chain_elem_t notif_chain_elem;
    memset(&notif_chain_elem, 0, sizeof(notif_chain_elem_t));
    notif_chain_elem.client_pid = getpid();
    notif_chain_comm_channel_t *
        notif_chain_comm_channel = &notif_chain_elem.notif_chain_comm_channel;
    notif_chain_comm_channel->notif_ch_type = NOTIF_C_CALLBACKS;
    NOTIF_CHAIN_ELEM_APP_CB(notif_chain_comm_channel) = test_cb;
    notif_chain_subscribe("rt table update", &notif_chain_elem);

    /* Start the publisher database
     * mgmt Operations*/
    main_menu(&rt);
    return 0;
}
