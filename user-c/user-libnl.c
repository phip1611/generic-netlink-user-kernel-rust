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

int main(void) {
    int res;

    // Allocate new netlink socket in memory.
    struct nl_sock * socket = nl_socket_alloc();

    // connect so that genl_ctrl_resolve can retrieve data
    // equivalent to nl_connect(socket, NETLINK_GENERIC);
    genl_connect(socket);

    // retrieve family id (kernel module registered a netlink family via generic netlink)
    int family_id = genl_ctrl_resolve(socket, FAMILY_NAME);

    if (family_id < 0) {
        fprintf(stderr, "generic netlink family '" FAMILY_NAME "' NOT REGISTERED\n");
        nl_socket_free(socket);
        exit(-1);
    } else {
        printf("Family-ID of generic netlink family '" FAMILY_NAME "' is: %d\n", family_id);
    }

    // we connect with the family id
    nl_connect(socket, family_id);

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
            1, // the command
            0
    );

    NLA_PUT_STRING(msg, EXMPL_A_MSG, "Hello World from Userland with libnl & libnl-genl");
    res = nl_send_auto_complete(socket, msg);
    //nlmsg_free(msg); already done by nl_send_auto_complete
    if (res < 0) {
        fprintf(stderr, "sending message failed\n");
    }

    nl_socket_free(socket);
    return 0;

nla_put_failure: // referenced by NLA_PUT_STRING
    nlmsg_free(msg);
    return -1;
}
