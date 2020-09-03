#ifndef __RT_NOTIF__
#define __RT_NOTIF__

#include <stdlib.h>
#include <memory.h>
#include "notif.h"
#include "rt.h"
#include "rt_notif.h"

/* Client APIs to subscribe for a particular 
 * entry of a routing table maintained by publisher
 * */
bool
rt_threaded_client_subscribe_for_ip(
        char *notif_chain_name,
        char *ip,
        char mask,
        notif_chain_app_cb cb){

    notif_chain_elem_t notif_chain_elem;
    memset(&notif_chain_elem, 0, sizeof(notif_chain_elem_t));
    notif_chain_elem.client_pid = getpid();
    notif_chain_elem.notif_code = SUBS_TO_PUB_NOTIF_C_SUBSCRIBE;
    rt_entry_keys_t rt_entry_keys;
    strncpy(rt_entry_keys.dest, ip, 16);
    rt_entry_keys.mask = mask;
    notif_chain_elem.data.app_key_data = (void *)&rt_entry_keys;
    notif_chain_elem.data.app_key_data_size = sizeof(rt_entry_keys_t);
    notif_chain_comm_channel_t *
        notif_chain_comm_channel = &notif_chain_elem.notif_chain_comm_channel;
    notif_chain_comm_channel->notif_ch_type = NOTIF_C_CALLBACKS;
    NOTIF_CHAIN_ELEM_APP_CB(notif_chain_comm_channel) = cb;
    return notif_chain_subscribe(notif_chain_name, &notif_chain_elem);
}

bool
rt_inet_skt_client_subscribe_for_ip(
        char *notif_chain_name,
        char *ip,
        char mask,
        char *publisher_addr,
        uint16_t port_no);

bool
rt_unix_skt_client_subscribe_for_ip(
        char *notif_chain_name,
        char *ip,
        char mask,
        char *publisher_unix_skt_name);

bool
rt_msgq_client_subscribe_for_ip(
        char *notif_chain_name,
        char *ip,
        char mask,
        char *publisher_msgq_name);

/* Client APIs to unsubscribe for a particular 
 * entry of a routing table maintained by publisher
 * */
bool
rt_threaded_client_unsubscribe_for_ip(
        char *notif_chain_name,
        char *ip,
        char mask);

bool
rt_inet_skt_client_unsubscribe_for_ip(
        char *notif_chain_name,
        char *ip,
        char mask,
        char *publisher_addr,
        uint16_t port_no);

bool
rt_unix_skt_client_unsubscribe_for_ip(
        char *notif_chain_name,
        char *ip,
        char mask,
        char *publisher_unix_skt_name);

bool
rt_msgq_client_unsubscribe_for_ip(
        char *notif_chain_name,
        char *ip,
        char mask,
        char *publisher_msgq_name);

#endif /* __RT_NOTIF__ */
