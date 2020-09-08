// Code taken from 
// http://www.electronicsfaq.com/2014/02/generic-netlink-sockets-example-code.html
// on 2020-08-01

// Make sure to make familiar with netlink and generic netlink first!
// I personally also just know a few things! If something is wrong please let me know!

// This code is the user part written in C that talks via a generic netlink family called
// "CONTROL_EXMPL" to a linux kernel module. It doesn't use any generic netlink library but
// does everything on raw sockets.
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
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>

#include <linux/genetlink.h>

#define FAMILY_NAME "CONTROL_EXMPL"

// Generic macros for dealing with netlink sockets
#define GENLMSG_DATA(glh) ((void *)(NLMSG_DATA(glh) + GENL_HDRLEN))
#define GENLMSG_PAYLOAD(glh) (NLMSG_PAYLOAD(glh, 0) - GENL_HDRLEN)
#define NLA_DATA(na) ((void *)((char *)(na) + NLA_HDRLEN))

#define MESSAGE_TO_KERNEL "Hello World from C user program!"

//Variables used for netlink
int nl_fd;                     //netlink socket's file descriptor
struct sockaddr_nl nl_address; //netlink socket address
int nl_family_id;              //The family ID resolved by the netlink controller for this userspace program
int nl_rxtx_length;            //Number of bytes sent or received via send() or recv()
struct nlattr *nl_na;          //pointer to netlink attributes structure within the payload
struct
{ //memory for netlink request and response messages - headers are included
    struct nlmsghdr n;
    struct genlmsghdr g;
    char buf[256];
} nl_request_msg, nl_response_msg;

int main(void)
{
    //Step 1: Open the socket. Note that protocol = NETLINK_GENERIC
    nl_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
    if (nl_fd < 0) {
        perror("socket()");
        fprintf(stderr, "error creating socket\n");
        return -1;
    }

    //Step 2: Bind the socket.
    memset(&nl_address, 0, sizeof(nl_address));
    // tell the socket (nl_address) that we use NETLINK protocol
    nl_address.nl_family = AF_NETLINK;
    nl_address.nl_groups = 0;

    if (bind(nl_fd, (struct sockaddr *)&nl_address, sizeof(nl_address)) < 0) {
        perror("bind()");
        close(nl_fd);
        fprintf(stderr, "error binding socket\n");
        return -1;
    }

    //Step 3. Resolve the family ID corresponding to the string "CONTROL_EXMPL"

    // It works like this: the linux kernel creats a new netlink family
    // on the fly when the kernel module registers the generic netlink family. 
    // After that communication is done with the new, temporary netlink family id.

    //Populate the netlink header
    nl_request_msg.n.nlmsg_type = GENL_ID_CTRL;
    nl_request_msg.n.nlmsg_flags = NLM_F_REQUEST;
    nl_request_msg.n.nlmsg_seq = 0;
    nl_request_msg.n.nlmsg_pid = getpid();
    nl_request_msg.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    //Populate the payload's "family header" : which in our case is genlmsghdr
    nl_request_msg.g.cmd = CTRL_CMD_GETFAMILY;
    nl_request_msg.g.version = 0x1;
    //Populate the payload's "netlink attributes"
    nl_na = (struct nlattr *)GENLMSG_DATA(&nl_request_msg);
    nl_na->nla_type = CTRL_ATTR_FAMILY_NAME;
    nl_na->nla_len = strlen(FAMILY_NAME) + 1 + NLA_HDRLEN;
    strcpy(NLA_DATA(nl_na), FAMILY_NAME); //Family name length can be upto 16 chars including \0

    nl_request_msg.n.nlmsg_len += NLMSG_ALIGN(nl_na->nla_len);

    memset(&nl_address, 0, sizeof(nl_address));
    // tell the socket (nl_address) that we use NETLINK protocol
    nl_address.nl_family = AF_NETLINK;

    //Send the family ID request message to the netlink controller
    nl_rxtx_length = sendto(nl_fd, (char *)&nl_request_msg, nl_request_msg.n.nlmsg_len,
                            0, (struct sockaddr *)&nl_address, sizeof(nl_address));
    if (nl_rxtx_length != nl_request_msg.n.nlmsg_len) {
        close(nl_fd);
        fprintf(stderr, "error sending family id request\n");
        return -1;
    }

    //Wait for the response message
    nl_rxtx_length = recv(nl_fd, &nl_response_msg, sizeof(nl_response_msg), 0);
    if (nl_rxtx_length < 0) {
        fprintf(stderr, "error receiving family id request result\n");
        return -1;
    }

    //Validate response message
    if (!NLMSG_OK((&nl_response_msg.n), nl_rxtx_length)) {
        fprintf(stderr, "family ID request : invalid message\n");
        fprintf(stderr, "error validating family id request result: invalid length\n");
        return -1;
    }
    if (nl_response_msg.n.nlmsg_type == NLMSG_ERROR) { //error
        fprintf(stderr, "family ID request : receive error\n");
        fprintf(stderr, "error validating family id request result: receive error\n");
        return -1;
    }

    //Extract family ID
    nl_na = (struct nlattr *)GENLMSG_DATA(&nl_response_msg);
    nl_na = (struct nlattr *)((char *)nl_na + NLA_ALIGN(nl_na->nla_len));
    if (nl_na->nla_type == CTRL_ATTR_FAMILY_ID) {
        nl_family_id = *(__u16 *)NLA_DATA(nl_na);
    }

    printf("extracted family id is: %d\n", nl_family_id);

    //Step 4. Send own custom message
    memset(&nl_request_msg, 0, sizeof(nl_request_msg));
    memset(&nl_response_msg, 0, sizeof(nl_response_msg));

    nl_request_msg.n.nlmsg_len = NLMSG_LENGTH(GENL_HDRLEN);
    nl_request_msg.n.nlmsg_type = nl_family_id;
    nl_request_msg.n.nlmsg_flags = NLM_F_REQUEST;
    nl_request_msg.n.nlmsg_seq = 60;
    nl_request_msg.n.nlmsg_pid = getpid();
    nl_request_msg.g.cmd = 1; //corresponds to DOC_EXMPL_C_ECHO;

    nl_na = (struct nlattr *)GENLMSG_DATA(&nl_request_msg);
    nl_na->nla_type = 1;                                     // corresponds to DOC_EXMPL_A_MSG
    nl_na->nla_len = sizeof(MESSAGE_TO_KERNEL) + NLA_HDRLEN; //Message length
    memcpy(NLA_DATA(nl_na), MESSAGE_TO_KERNEL, sizeof(MESSAGE_TO_KERNEL));
    nl_request_msg.n.nlmsg_len += NLMSG_ALIGN(nl_na->nla_len);

    memset(&nl_address, 0, sizeof(nl_address));
    nl_address.nl_family = AF_NETLINK;

    //Send the custom message
    nl_rxtx_length = sendto(nl_fd, (char *)&nl_request_msg, nl_request_msg.n.nlmsg_len,
                            0, (struct sockaddr *)&nl_address, sizeof(nl_address));
    if (nl_rxtx_length != nl_request_msg.n.nlmsg_len) {
        close(nl_fd);
        fprintf(stderr, "error sending custom message\n");
        return -1;
    }
    printf("Sent to kernel: %s\n", MESSAGE_TO_KERNEL);

    //Receive reply from kernel
    nl_rxtx_length = recv(nl_fd, &nl_response_msg, sizeof(nl_response_msg), 0);
    if (nl_rxtx_length < 0) {
        fprintf(stderr, "error receiving custom message result: no length\n");
        close(nl_fd);
        return -1;
    }
    //Validate response message
    if (nl_response_msg.n.nlmsg_type == NLMSG_ERROR) { //Error
        printf("Error while receiving reply from kernel: NACK Received\n");
        close(nl_fd);
        fprintf(stderr, "error receiving custom message result\n");
        return -1;
    }
    if (!NLMSG_OK((&nl_response_msg.n), nl_rxtx_length)) {
        printf("Error while receiving reply from kernel: Invalid Message\n");
        close(nl_fd);
        return -1;
    }

    //Parse the reply message
    nl_rxtx_length = GENLMSG_PAYLOAD(&nl_response_msg.n);
    nl_na = (struct nlattr *)GENLMSG_DATA(&nl_response_msg);
    printf("Kernel replied: %s\n", (char *)NLA_DATA(nl_na));

    //Step 5. Close the socket and quit
    close(nl_fd);
    return 0;
}