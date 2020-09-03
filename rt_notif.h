#ifndef __RT_NOTIF__
#define __RT_NOTIF__

#include <stdbool.h>
#include <stdint.h>

/* Client APIs to subscribe for a particular 
 * entry of a routing table maintained by publisher
 * */
bool
rt_threaded_client_subscribe_for_ip(
        char *notif_chain_name,
        char *ip,
        char mask,
        notif_chain_app_cb cb);

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
