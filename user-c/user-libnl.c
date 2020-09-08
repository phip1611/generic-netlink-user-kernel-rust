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

// "libnl" (core)
#include <netlink/netlink.h>
// "libnl-genl" (genl extension on top of core)
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h> // for genl_ctrl_ functions

#define FAMILY_NAME "CONTROL_EXMPL"

int main(void) {
    // Allocate new netlink socket in memory.
    struct nl_sock * socket = nl_socket_alloc();

    // connect so that genl_ctrl_resolve can retrieve data
    // equivalent to nl_connect(socket, NETLINK_GENERIC);
    genl_connect(socket);

    // retrieve family id (kernel module registered a netlink family via generic netlink)
    int family_id = genl_ctrl_resolve(socket, FAMILY_NAME);

    if (family_id < 0) {
        printf("generic netlink family '" FAMILY_NAME "' NOT REGISTERED\n");
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
    /*void * user_hdr = genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, family_id,
                           sizeof(struct my_hdr), 0, 1, 0);
    if (!user_hdr)
        // ERROR
*/
    nlmsg_free(msg);

    nl_socket_free(socket);
}
