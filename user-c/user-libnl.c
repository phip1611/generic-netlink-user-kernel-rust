/* Copyright 2021 Philipp Schuster
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of 
 * this software and associated documentation files (the "Software"), to deal in the 
 * Software without restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies
 * or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* Userland component written in C, that uses libnl to talk to a custom Netlink
 * family via Generic Netlink. The family is called "gnl_foobar_xmpl" and the
 * kernel module must be loaded first. Otherwise the family doesn't exist.
 */

#include <stdio.h>

#include <netlink/attr.h>
// "libnl" (core)
#include <netlink/netlink.h>
// "libnl-genl" (genl extension on top of core)
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h> // for genl_ctrl_ functions
// if we want to use functions flags like "NLM_F_ACK"
#include <linux/netlink.h>
#include <sys/time.h>

// data/vars/enums/properties that describes our protocol that we implement
// on top of generic netlink (like functions we want to trigger on the receiving side)
#include "gnl_foobar_xmpl.h"

#define MESSAGE_TO_KERNEL "Hello World from Userland with libnl & libnl-genl"

#define LOG_PREFIX "[User-C-libnl] "

// netlink family id of the netlink family we want to use
int family_id = -1;

// Callback function for all received netlink messages
int nl_callback(struct nl_msg* recv_msg, void* arg)
{
    // pointer to actual returned msg
    struct nlmsghdr * ret_hdr = nlmsg_hdr(recv_msg);

    // array that is a mapping from received attribute to actual data or NULL
    // (we can only send an specific attribute once per msg)
    struct nlattr * tb_msg[GNL_FOOBAR_XMPL_A_MAX + 1];

    // nlmsg_type is either family id number for "good" messages
    // or NLMSG_ERROR for bad massages.
    if (ret_hdr->nlmsg_type == NLMSG_ERROR) {
        fprintf(stderr, LOG_PREFIX "Received NLMSG_ERROR message!\n");
        return NL_STOP;
    }

    // Pointer to message payload
    struct genlmsghdr *gnlh = (struct genlmsghdr*) nlmsg_data(ret_hdr);

    // Create attribute index based on a stream of attributes.
    nla_parse(tb_msg, // Index array to be filled (maxtype+1 elements)
              GNL_FOOBAR_XMPL_A_MAX, // Maximum attribute type expected and accepted.
              genlmsg_attrdata(gnlh, 0), // Head of attribute stream
              genlmsg_attrlen(gnlh, 0), // 	Length of attribute stream
              NULL // GNlFoobarXmplAttribute validation policy
    );

    // check if a msg was actually received
    if (tb_msg[GNL_FOOBAR_XMPL_A_MSG]) {
        // parse it as string
        char * payload_msg = nla_get_string(tb_msg[GNL_FOOBAR_XMPL_A_MSG]);
        // printf(LOG_PREFIX "Kernel replied: %s\n", payload_msg);
    }

    return NL_OK;
}

int main(void) {
    // ############################################################################################
    // ########## Step 1: Connect via generic netlink
    struct timeval st, et;
    gettimeofday(&st,NULL);
    for (int i = 0; i < 100000; i++) {

        // Allocate new netlink socket in memory.
        struct nl_sock *socket = nl_socket_alloc();

        // connect so that genl_ctrl_resolve can retrieve data
        // equivalent to nl_connect(socket, NETLINK_GENERIC);
        genl_connect(socket);

        // retrieve family id (kernel module registered a netlink family via generic netlink)
        family_id = genl_ctrl_resolve(socket, FAMILY_NAME);

        if (family_id < 0) {
            fprintf(stderr, LOG_PREFIX "generic netlink family '" FAMILY_NAME "' NOT REGISTERED\n");
            nl_socket_free(socket);
            exit(-1);
        } else {
            // printf(LOG_PREFIX "Family-ID of generic netlink family '" FAMILY_NAME "' is: %d\n", family_id);
        }

        // ############################################################################################
        // ########## Step 2: Sending Data

        // there is no reconnect with the new family necessary because this is already done by
        // genl_ctrl_resolve(); see genl/ctrl.c
        // nl_connect(socket, family_id);

        // attach a callback
        nl_socket_modify_cb(socket,
                // btw: NL_CB_VALID doesn't work here;
                // perhaps our kernel module sends somehow incomplete msgs?!
                            NL_CB_MSG_IN, //  Called for every message received
                            NL_CB_CUSTOM, // custom handler specified by us
                            nl_callback, // function
                            NULL // no argument to be passed to callback function
        );

        // we build a netlink package
        // it's payload is the generic netlink header with its data
        // nl message with default size
        struct nl_msg *msg = nlmsg_alloc();
        genlmsg_put(
                msg,  // memory buffer
                // Port ID. Not necessarily the process id of the current process. This field
                // could be used to identify different points or threads inside your application
                // that send data to the kernel. This has nothing to do with "routing" the packet to
                // the kernel, because this is done by the socket itself
                NL_AUTO_PORT, // auto assign current pid
                // It is up to you if you want to split a data transfer into multiple sequences. (application specific)
                NL_AUTO_SEQ, // begin wit seq number 0
                family_id, // family id
                0, // no additional user header
                // You can use flags in an application specific way (e.g. ACK flag). It is up to you
                // if you check against flags in your Kernel module. It is required to add NLM_F_REQUEST,
                // otherwise the Kernel doesn't route the packet to the right Netlink callback handler
                // in your Kernel module. This might result in a deadlock on the socket if an expected
                // reply you are waiting for in a blocking way is never received.
                // Kernel reference: https://elixir.bootlin.com/linux/v5.10.16/source/net/netlink/af_netlink.c#L2487
                //
                // libnl specific: always adds NLM_F_REQUEST
                // see <libnl>/nl.c#nl_complete_msg()
                NLM_F_REQUEST,
                // cmd in Generic Netlink Header
                GNL_FOOBAR_XMPL_C_ECHO, // the command we want to trigger on the receiving side
                // You can evolve your application over time using different versions or ignore it.
                // Application specific; receiver can check this value and to specific logic
                1
        );

        NLA_PUT_STRING(msg, GNL_FOOBAR_XMPL_A_MSG, MESSAGE_TO_KERNEL);
        int res = nl_send_auto(socket, msg);
        nlmsg_free(msg);
        if (res < 0) {
            fprintf(stderr, LOG_PREFIX "sending message failed\n");
        } else {
            // printf(LOG_PREFIX "Sent to kernel: '%s'\n", MESSAGE_TO_KERNEL);
        }

        // ############################################################################################
        // ########## Step 3: receive data

        // wait for received messages and handle them according to our callback handlers
        nl_recvmsgs_default(socket);

        nl_socket_free(socket);
        continue;

    nla_put_failure: // referenced by NLA_PUT_STRING
        nlmsg_free(msg);
        return -1;
    }
    gettimeofday(&et,NULL);
    long avg_duration_us = (((et.tv_sec - st.tv_sec) * 1000000) + (et.tv_usec - st.tv_usec)) / 100000;
    printf(LOG_PREFIX "Took on average %ldµs\n", avg_duration_us);
    return 0;
}
