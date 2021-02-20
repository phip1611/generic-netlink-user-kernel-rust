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

/* Code inspired by this post, but I made multiple changes:
 * http://www.electronicsfaq.com/2014/02/generic-netlink-sockets-example-code.html
 * Probably by Anurag Chugh (not sure): https://www.blogger.com/profile/15390575283968794206
 */

/* Userland component written in C, that uses pure sockets to talk to a custom Netlink
 * family via Generic Netlink. The family is called "gnl_foobar_xmpl" and the
 * kernel module must be loaded first. Otherwise the family doesn't exist.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>

#include <linux/genetlink.h>

// data/vars/enums/properties that describes our protocol that we implement
// on top of generic netlink (like functions we want to trigger on the receiving side)
#include "gnl_foobar_xmpl_prop.h"

#define LOG_PREFIX "[User-C-Pure] "

// Generic macros for dealing with netlink sockets
#define GENLMSG_DATA(glh) ((void *)(NLMSG_DATA(glh) + GENL_HDRLEN))
#define GENLMSG_PAYLOAD(glh) (NLMSG_PAYLOAD(glh, 0) - GENL_HDRLEN)
#define NLA_DATA(na) ((void *)((char *)(na) + NLA_HDRLEN))

#define MESSAGE_TO_KERNEL "Hello World from C user program (using raw sockets)!"

/**
 * Structure describing the memory layout of a Generic Netlink layout.
 * The buffer size of 256 byte here is chosen at will and for simplicity.
 */
struct generic_netlink_msg {
    /** Netlink header comes first. */
    struct nlmsghdr n;
    /** Afterwards the Generic Netlink header */
    struct genlmsghdr g;
    /** Custom data. Space for Netlink Attributes. */
    char buf[256];
};

// Global Variables used for our Netlink example
/** Netlink socket's file descriptor. */
int nl_fd;
/** Netlink socket address */
struct sockaddr_nl nl_address;
/** The family ID resolved by Generic Netlink control interface. Assigned when the kernel module registers the Family */
int nl_family_id = -1;
/** Number of bytes sent or received via send() or recv() */
int nl_rxtx_length;
/** Pointer to Netlink attributes structure within the payload */
struct nlattr *nl_na;
/** Memory for Netlink request message. */
struct generic_netlink_msg nl_request_msg;
/** Memory for Netlink response message. */
struct generic_netlink_msg nl_response_msg;

// Comments on function body below.
int open_and_bind_socket();
// Comments on function body below.
int resolve_family_id_by_name();
// Comments on function body below.
int send_echo_msg_and_get_reply();

int main(void)
{
    // go through the functions in order one by one and try to understand as good as you can :) good luck!
    // The comments in `send_echo_msg_and_get_reply()` are more detailed than in `resolve_family_id_by_name()`
    // because the first is the actual IPC with kernel while the latter is mandatory setup code.

    open_and_bind_socket();
    resolve_family_id_by_name();

    printf(LOG_PREFIX "extracted family id is: %d\n", nl_family_id);

    // no we have family id; now we can actually talk to our custom Netlink family
    send_echo_msg_and_get_reply();

    // Step 5. Close the socket and quit
    close(nl_fd);
    return 0;
}

/**
 * Opens and binds the socket to Netlink
 *
 * @return < 0 on failure or 0 on success.
 */
int open_and_bind_socket() {
    // Step 1: Open the socket. Note that protocol = NETLINK_GENERIC in the address family of Netlink (AF_NETLINK)
    nl_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
    if (nl_fd < 0) {
        perror(LOG_PREFIX "socket()");
        fprintf(stderr, LOG_PREFIX "error creating socket\n");
        return -1;
    }

    // Step 2: Bind the socket.
    memset(&nl_address, 0, sizeof(nl_address));
    // tell the socket (nl_address) that we use NETLINK address family
    nl_address.nl_family = AF_NETLINK;

    if (bind(nl_fd, (struct sockaddr *)&nl_address, sizeof(nl_address)) < 0) {
        perror(LOG_PREFIX "bind()");
        close(nl_fd);
        fprintf(stderr, LOG_PREFIX "error binding socket\n");
        return -1;
    }
    return 0;
}

/**
 * Resolves the id of the Netlink family `FAMILY_NAME` from `gnl_foobar_xmpl.prop.h`
 * using Generic Netlink control interface,
 *
 * @return < 0 on failure or 0 on success.
 */
int resolve_family_id_by_name() {
    // Step 3. Resolve the family ID corresponding to the macro `FAMILY_NAME` from `gnl_foobar_xmpl_prop.h`
    // We use come CTRL mechanisms that are part of the Generic Netlink infrastructure.
    // This part is usually behind a nice abstraction in each library, something like
    // `resolve_family_id_by_name()`. You can find more in
    // Linux code: https://elixir.bootlin.com/linux/latest/source/include/uapi/linux/genetlink.h#L30

    // This is required because before we can actually talk to our custom Netlink family, we need the numeric id.
    // Next we ask the Kernel for the ID.

    // Populate the netlink header
    nl_request_msg.n.nlmsg_type = GENL_ID_CTRL;
    // NLM_F_REQUEST is REQUIRED for kernel requests, otherwise the packet is rejected!
    // Kernel reference: https://elixir.bootlin.com/linux/v5.10.16/source/net/netlink/af_netlink.c#L2487
    nl_request_msg.n.nlmsg_flags = NLM_F_REQUEST;
    nl_request_msg.n.nlmsg_seq = 0;
    nl_request_msg.n.nlmsg_pid = getpid();
    nl_request_msg.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    // Populate the payload's "family header" : which in our case is genlmsghdr
    nl_request_msg.g.cmd = CTRL_CMD_GETFAMILY;
    nl_request_msg.g.version = 1;
    // Populate the payload's "netlink attributes"
    nl_na = (struct nlattr *)GENLMSG_DATA(&nl_request_msg);
    nl_na->nla_type = CTRL_ATTR_FAMILY_NAME;
    nl_na->nla_len = strlen(FAMILY_NAME) + 1 + NLA_HDRLEN;
    strcpy(NLA_DATA(nl_na), FAMILY_NAME); //Family name length can be upto 16 chars including \0

    nl_request_msg.n.nlmsg_len += NLMSG_ALIGN(nl_na->nla_len);

    // tell the socket (nl_address) that we use NETLINK address family and that we target
    // the kernel (pid = 0)
    nl_address.nl_family = AF_NETLINK;
    nl_address.nl_pid = 0; // <-- we target the kernel; kernel pid is 0
    nl_address.nl_groups = 0; // we don't use multicast groups
    nl_address.nl_pad = 0;

    // Send the family ID request message to the netlink controller
    nl_rxtx_length = sendto(nl_fd, (char *)&nl_request_msg, nl_request_msg.n.nlmsg_len,
                            0, (struct sockaddr *)&nl_address, sizeof(nl_address));
    if (nl_rxtx_length != nl_request_msg.n.nlmsg_len) {
        close(nl_fd);
        fprintf(stderr, LOG_PREFIX "error sending family id request\n");
        return -1;
    }

    // Wait for the response message
    nl_rxtx_length = recv(nl_fd, &nl_response_msg, sizeof(nl_response_msg), 0);
    if (nl_rxtx_length < 0) {
        fprintf(stderr, LOG_PREFIX "error receiving family id request result\n");
        return -1;
    }

    // Validate response message
    if (!NLMSG_OK((&nl_response_msg.n), nl_rxtx_length)) {
        fprintf(stderr, LOG_PREFIX "family ID request : invalid message\n");
        fprintf(stderr, LOG_PREFIX "error validating family id request result: invalid length\n");
        return -1;
    }
    if (nl_response_msg.n.nlmsg_type == NLMSG_ERROR) { // error
        fprintf(stderr, LOG_PREFIX "family ID request : receive error\n");
        fprintf(stderr, LOG_PREFIX "error validating family id request result: receive error\n");
        return -1;
    }

    // Extract family ID
    nl_na = (struct nlattr *)GENLMSG_DATA(&nl_response_msg);
    nl_na = (struct nlattr *)((char *)nl_na + NLA_ALIGN(nl_na->nla_len));
    if (nl_na->nla_type == CTRL_ATTR_FAMILY_ID) {
        nl_family_id = *(__u16 *)NLA_DATA(nl_na);
    }

    return 0;
}

/**
 * Sends an echo request and receives the echoed message.
 *
 * @return < 0 on failure or 0 on success.
 */
int send_echo_msg_and_get_reply() {
    // Step 4. Send own custom message
    memset(&nl_request_msg, 0, sizeof(nl_request_msg));
    memset(&nl_response_msg, 0, sizeof(nl_response_msg));

    nl_request_msg.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    // This is NOT the property for proper "routing" of the netlink message (that is located in the socket struct).
    // This is family id for "good" messages or NLMSG_ERROR (0x2) for error messages
    nl_request_msg.n.nlmsg_type = nl_family_id;

    // You can use flags in an application specific way, e.g. NLM_F_CREATE or NLM_F_EXCL.
    // Some flags have pre-defined functionality, like NLM_F_DUMP or NLM_F_ACK (Netlink will
    // do actions before your callback in the kernel can start its processing). You can see
    // some examples in https://elixir.bootlin.com/linux/v5.10.16/source/net/netlink/af_netlink.c
    //
    // NLM_F_REQUEST is REQUIRED for kernel requests, otherwise the packet is rejected!
    // Kernel reference: https://elixir.bootlin.com/linux/v5.10.16/source/net/netlink/af_netlink.c#L2487
    //
    // if you add "NLM_F_DUMP" flag, the .dumpit callback will be invoked in the kernel
    nl_request_msg.n.nlmsg_flags = NLM_F_REQUEST;
    // It is up to you if you want to split a data transfer into multiple sequences. (application specific)
    nl_request_msg.n.nlmsg_seq = 0;
    // Port ID. Not necessarily the process id of the current process. This field
    // could be used to identify different points or threads inside your application
    // that send data to the kernel. This has nothing to do with "routing" the packet to
    // the kernel, because this is done by the socket itself
    nl_request_msg.n.nlmsg_pid = getpid();
    // nl_request_msg.g.cmd = GNL_FOOBAR_XMPL_C_REPLY_WITH_NLMSG_ERR;
    nl_request_msg.g.cmd = GNL_FOOBAR_XMPL_C_ECHO;
    // You can evolve your application over time using different versions or ignore it.
    // Application specific; receiver can check this value and do specific logic.
    nl_request_msg.g.version = 1; // app specific; we don't use this on the receiving side in our example

    nl_na = (struct nlattr *)GENLMSG_DATA(&nl_request_msg);
    nl_na->nla_type = GNL_FOOBAR_XMPL_A_MSG;
    nl_na->nla_len = sizeof(MESSAGE_TO_KERNEL) + NLA_HDRLEN; // Message length
    memcpy(NLA_DATA(nl_na), MESSAGE_TO_KERNEL, sizeof(MESSAGE_TO_KERNEL));
    nl_request_msg.n.nlmsg_len += NLMSG_ALIGN(nl_na->nla_len);

    // Send the custom message
    nl_rxtx_length = sendto(nl_fd, (char *)&nl_request_msg, nl_request_msg.n.nlmsg_len,
                            0, (struct sockaddr *)&nl_address, sizeof(nl_address));
    if (nl_rxtx_length != nl_request_msg.n.nlmsg_len) {
        close(nl_fd);
        fprintf(stderr, LOG_PREFIX "error sending custom message\n");
        return -1;
    }
    printf(LOG_PREFIX "Sent to kernel: %s\n", MESSAGE_TO_KERNEL);

    // Receive reply from kernel
    nl_rxtx_length = recv(nl_fd, &nl_response_msg, sizeof(nl_response_msg), 0);
    if (nl_rxtx_length < 0) {
        fprintf(stderr, LOG_PREFIX "error receiving custom message result: no length\n");
        close(nl_fd);
        return -1;
    }

    /* ############################################################################### */
    // With this code you can print the nlmsg including all payload byte by byte
    // useful for debugging, developing libs or understanding netlink on the lowest level!
    // char unsigned * byte_ptr = (unsigned char *) &(nl_response_msg.n);
    // printf("let buf = vec![\n");
    // int len = nl_response_msg.n.nlmsg_len;
    // for (int i = 0; i < len; i++) {
    //     printf("  0x%x, \n", byte_ptr[i]);
    // }
    // printf("];\n");
    /* ############################################################################### */

    // Validate response message
    if (nl_response_msg.n.nlmsg_type == NLMSG_ERROR) { //E rror
        printf(LOG_PREFIX "Error while receiving reply from kernel: NACK Received\n");
        close(nl_fd);
        fprintf(stderr, LOG_PREFIX "error receiving custom message result\n");
        return -1;
    }

    // check if format is good
    if (!NLMSG_OK((&nl_response_msg.n), nl_rxtx_length)) {
        printf(LOG_PREFIX "Error while receiving reply from kernel: Invalid Message\n");
        close(nl_fd);
        return -1;
    }

    // Parse the reply message
    nl_rxtx_length = GENLMSG_PAYLOAD(&nl_response_msg.n);
    nl_na = (struct nlattr *)GENLMSG_DATA(&nl_response_msg);
    printf(LOG_PREFIX "Kernel replied: '%s'\n", (char *)NLA_DATA(nl_na));

    return 0;
}