/*
 * =====================================================================================
 *
 *       Filename:  msgq_subs.c
 *
 *    Description: This file represents the MsgQ subscriber 
 *
 *        Version:  1.0
 *        Created:  09/10/2020 12:05:39 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ABHISHEK SAGAR (), sachinites@gmail.com
 *   Organization:  Juniper Networks
 *
 * =====================================================================================
 */

/* Visit : www.csepracticals.com */

#include <string.h>
#include "notif.h"
#include "rt.h"

int
main(int argc, char** argv){

	rt_entry_keys_t rt_entry_keys;
	
	const char *msgq_name = "msgq1";

	strncpy(rt_entry_keys.dest, "122.1.1.3", 16);
	rt_entry_keys.mask = 32;

	notif_chain_subscribe_msgq(
		"notif_chain_rt_table",
		&rt_entry_keys,
		sizeof(rt_entry_keys_t),
		1234,
		msgq_name,
		"127.0.0.1",
		2000);
	return 0;
}
