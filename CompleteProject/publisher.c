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
#include "notif.h"
#include "rt.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "network_utils.h"

extern void
create_subscriber_thread();

static notif_chain_t notif_chain;

static int choice;

void 
main_menu(rt_table_t *rt){


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
        printf("5. Notif Chain Dump\n");
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
                    notif_chain_elem_t notif_chain_elem;
                    memset(&notif_chain_elem, 0, sizeof(notif_chain_elem_t));
                    notif_chain_elem.client_id = 0; /* Not required */
                    notif_chain_elem.notif_code = PUB_TO_SUBS_NOTIF_C_CREATE;
                    rt_entry_keys_t rt_entry_keys;
                    strncpy(rt_entry_keys.dest, dest, 16);
                    rt_entry_keys.mask = mask;
                    notif_chain_elem.data.app_key_data = (void *)&rt_entry_keys;
                    notif_chain_elem.data.app_key_data_size = sizeof(rt_entry_keys_t);
                    notif_chain_elem.data.is_alloc_app_data_to_notify = false;
                    char data[256];
                    int rc = 0;
                    memset(data, 0, sizeof(256));
                    strncpy(data, oif, sizeof(oif));
                    rc = sizeof(oif);
                    strncpy(data + rc, gw, sizeof(gw));
                    rc += sizeof(gw);
                    notif_chain_elem.data.app_data_to_notify = data;
                    notif_chain_elem.data.app_data_to_notify_size = rc;
                    notif_chain_invoke(&notif_chain, &notif_chain_elem);
                }  
            break;
            case 2:
            case 3:
            case 4:
                rt_dump_rt_table(rt);
                break;
            case 5:
               notif_chain_dump(&notif_chain);
               break;
            default:
                exit(0);
        }
    }
}

int
rt_entry_comp_fn(void *key_data1, 
                 uint32_t key_data1_size, 
                 void *key_data2, 
                 uint32_t key_data2_size){

    rt_entry_keys_t *rt_entry_keys1 = (rt_entry_keys_t *)key_data1;
    rt_entry_keys_t *rt_entry_keys2 = (rt_entry_keys_t *)key_data2;

    if(strncmp(rt_entry_keys1->dest, rt_entry_keys2->dest, 16) == 0 &&
        rt_entry_keys1->mask == rt_entry_keys2->mask){
        return 0;
    }

    return -1;
}

static char *
rt_entry_keys_print_fn(void *keys, uint32_t key_size,
                      char *output_buff, uint32_t buff_size){

    if(!keys || !key_size){
        sprintf(output_buff, "Dest = Nil");
        return output_buff;
    }
    rt_entry_keys_t *rt_entry_keys = (rt_entry_keys_t *)keys;
    sprintf(output_buff, "Dest = %s/%d", 
        rt_entry_keys->dest, rt_entry_keys->mask);
    return output_buff;
}

void
tcp_subscriber_join_notification(char *ip_addr,
								 uint32_t tcp_port_no){

	printf("%s() Called \n", __FUNCTION__);
}

void
tcp_subscriber_killed_notification(char *ip_addr,
								   uint32_t tcp_port_no){

	printf("%s() Called \n", __FUNCTION__);
}

int
main(int argc, char **argv){

    /*Published is in-charge of the Data source
     * i.e. Routing Table in this example*/
    rt_table_t rt;
    rt_init_rt_table(&rt);

    /* Publisher is creating a Notification Chain for
     * its data Source*/
    notif_chain_init(&notif_chain,
        "notif_chain_rt_table",
        rt_entry_comp_fn,
        rt_entry_keys_print_fn);

    /* We will create a thread which will be act 
     * as a subscriber to publisher to allow 
     * test notification chains using callbacks.
     * Notif chains uses callbacks to notify the subscribers only
     * for local subscribers i.e. subscribers running as a thread
     * of publisher process*/
     create_subscriber_thread(1);
     create_subscriber_thread(2);
     create_subscriber_thread(3);
    
    /* Publisher needs to start the the separate thread so
     * that it can listen to remote subscriber's request on UDP socket.
     * Remote subscribers are those which runs as a separate process
     * on same or remote machine*/
     network_start_udp_pkt_receiver_thread(
			"127.0.0.1",
			2000,
			notif_chain_process_remote_subscriber_request);	
     
    /* Publisher needs to start the the separate thread so
     * that it can listen to remote subscriber's request on TCP socket.
     * Remote subscribers are those which runs as a separate process
     * on same or remote machine*/
	 network_start_tcp_pkt_receiver_thread(
			"127.0.0.1",
			2002,
			notif_chain_process_remote_subscriber_request,
			tcp_subscriber_join_notification,
			tcp_subscriber_killed_notification);	

    /* Start the publisher database
     * mgmt Operations*/
    main_menu(&rt);
    return 0;
}

