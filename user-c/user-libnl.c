// This code is the user part written in C that talks via a generic netlink family called
// "CONTROL_EXMPL" to a linux kernel module. It uses 'libnl'.
//
// Requires packages "libnl-3-dev" and "libnl-genl-3-dev"
//
// Each netlink packet has a generic netlink packet as it's payload. In the generic netlink
// package one can define several attributes (type to value mappings).
// 
// Workflow is the following:
// 0) kernel module registers generic netlink family by name which creates a new 
//    virtual/temporary netlink family (a number)
// 1) client request family id of "CONTROL_EXMPL"
// 2) client sends data to retrieved family id.

#include <stdio.h>

#include <netlink/attr.h>
// "libnl" (core)
#include <netlink/netlink.h>
// "libnl-genl" (genl extension on top of core)
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h> // for genl_ctrl_ functions
// if we want to use functions flags like "NLM_F_ACK"
#include <linux/netlink.h>

// data/vars/enums/properties that describes our protocol that we implement
// on top of generic netlink (like functions we want to trigger on the receiving side)
#include "exmpl-protocol-nl.h"

#define MESSAGE_TO_KERNEL "Hello World from Userland with libnl & libnl-genl"

// netlink family id of the netlink family we want to use
int family_id = -1;

// Callback function for all received netlink messages
int nl_callback(struct nl_msg* recv_msg, void* arg)
{
    // pointer to actual returned msg
    struct nlmsghdr * ret_hdr = nlmsg_hdr(recv_msg);

    // array that is a mapping from received attribute to actual data or NULL
    // (we can only send an specific attribute once per msg)
    struct nlattr * tb_msg[EXMPL_A_MAX + 1];

    // nlmsg_type is either family id number for "good" messages
    // or NLMSG_ERROR for bad massages.
    if (ret_hdr->nlmsg_type == NLMSG_ERROR) {
        fprintf(stderr, "Received NLMSG_ERROR message!\n");
        return NL_STOP;
    }

    // Pointer to message payload
    struct genlmsghdr *gnlh = (struct genlmsghdr*) nlmsg_data(ret_hdr);

    // Create attribute index based on a stream of attributes.
    nla_parse(tb_msg, // Index array to be filled (maxtype+1 elements)
              EXMPL_A_MAX, // Maximum attribute type expected and accepted.
              genlmsg_attrdata(gnlh, 0), // Head of attribute stream
              genlmsg_attrlen(gnlh, 0), // 	Length of attribute stream
              NULL // Attribute validation policy
    );

    // check if a msg was actually received
    if (tb_msg[EXMPL_A_MSG]) {
        // parse it as string
        char * payload_msg = nla_get_string(tb_msg[EXMPL_A_MSG]);
        printf("Kernel replied: %s\n", payload_msg);
    }

    return NL_OK;
}

int main(void) {
    // ############################################################################################
    // ########## Step 1: Connect via generic netlink

    // Allocate new netlink socket in memory.
    struct nl_sock * socket = nl_socket_alloc();

    // connect so that genl_ctrl_resolve can retrieve data
    // equivalent to nl_connect(socket, NETLINK_GENERIC);
    genl_connect(socket);

    // retrieve family id (kernel module registered a netlink family via generic netlink)
    family_id = genl_ctrl_resolve(socket, FAMILY_NAME);

    if (family_id < 0) {
        fprintf(stderr, "generic netlink family '" FAMILY_NAME "' NOT REGISTERED\n");
        nl_socket_free(socket);
        exit(-1);
    } else {
        printf("Family-ID of generic netlink family '" FAMILY_NAME "' is: %d\n", family_id);
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
    struct nl_msg * msg = nlmsg_alloc();
    genlmsg_put(
            msg,  // nl msg
            NL_AUTO_PORT, // auto assign current pid
            NL_AUTO_SEQ, // begin wit hseq number 0
            family_id, // family id
            0, // no additional user header
            // flags; my example doesn't use this flags; kernel module ignores them whn
            // parsing the message; it's just here to show you how it would work
            NLM_F_ACK | NLM_F_ECHO,
            // EXMPL_C_ECHO_FAIL, // the command we want to trigger on the receiving side
            EXMPL_C_ECHO, // the command we want to trigger on the receiving side
            0 // Interface version (I don't know why this is useful; perhaps receiving side can adjust actions if
            // functionality evolves during development and multiple releases)
    );

    NLA_PUT_STRING(msg, EXMPL_A_MSG, MESSAGE_TO_KERNEL);
    int res = nl_send_auto(socket, msg);
    nlmsg_free(msg);
    if (res < 0) {
        fprintf(stderr, "sending message failed\n");
    } else {
        printf("Sent to kernel: %s\n", MESSAGE_TO_KERNEL);
    }

    // ############################################################################################
    // ########## Step 3: receive data

    // wait for received messages and handle them according to our callback handlers
    nl_recvmsgs_default(socket);


    nl_socket_free(socket);
    return 0;

nla_put_failure: // referenced by NLA_PUT_STRING
    nlmsg_free(msg);
    return -1;
}
