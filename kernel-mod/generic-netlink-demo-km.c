#include <linux/module.h>
#include <linux/fs.h>
#include <net/sock.h>
#include <net/genetlink.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>

// data/vars/enums/properties that describes our protocol that we implement
// on top of generic netlink (like functions we want to trigger on the receiving side)
#include "exmpl-protocol-nl.h"

/* attribute policy: defines which attribute has which type (e.g int, char * etc)
 * possible values defined in net/netlink.h 
 */
static struct nla_policy doc_exmpl_genl_policy[EXMPL_A_MAX + 1] = {
    [EXMPL_A_MSG] = {.type = NLA_NUL_STRING},
};

// callback function for echo command
int doc_exmpl_echo(struct sk_buff *, struct genl_info *);
// callback function for echo command, that replies with an error message
int doc_exmpl_echo_fail(struct sk_buff *, struct genl_info *);

// Array with all operations that out protocol on top of generic netlink
// supports. An operation is the glue between a command (number) and the
// corresponding callback function.
struct genl_ops ops[__EXMPL_C_MAX] = {
    {
        .cmd = EXMPL_C_ECHO,
        .flags = 0,
        .doit = doc_exmpl_echo,
        .dumpit = NULL,
    },
    {
        .cmd = EXMPL_C_ECHO_FAIL,
        .flags = 0,
        .doit = doc_exmpl_echo_fail,
        .dumpit = NULL,
    }
};

//family definition
static struct genl_family doc_exmpl_gnl_family = {
    .id = 0, // automatically assign an id
    .hdrsize = 0, // we don't use custom additional header info
    .name = FAMILY_NAME, // The name of this family, used by userspace application
    .version = 1,   // family specific version number
    .maxattr = EXMPL_A_MAX, // should also be the bounds check for policy
    .ops = ops, // delegates all incoming requests to callback functions
    .n_ops = EXMPL_C_MAX,
    .policy = doc_exmpl_genl_policy,
    .module = THIS_MODULE,
};

// An echo command, receives a message, prints it and sends another message back
int doc_exmpl_echo(struct sk_buff *skb_2, struct genl_info *info)
{    
    struct nlattr *na;
    struct sk_buff *skb;
    int rc;
    void * msg_head;
    char * recv_msg;

    printk(KERN_INFO "generic-netlink-demo-km: doc_exmpl_echo() invoked\n");

    if (info == NULL) {
        printk(KERN_INFO "An error occurred in doc_exmpl_echo:\n");
        return -1;
    }

    /* For each attribute there is an index in info->attrs which points to a nlattr structure
     * in this structure the data is given
     */
    na = info->attrs[EXMPL_A_MSG];
    if (na) {
        recv_msg = (char *)nla_data(na);
        if (recv_msg == NULL) {
            printk(KERN_INFO "error while receiving data\n");
        }
        else {
            printk(KERN_INFO "received: %s\n", recv_msg);
        }
    } else {
        printk(KERN_INFO "no info->attrs %i\n", EXMPL_A_MSG);
        return -1; // we return here because we expect to recv a msg
    }

    // Send a message back
    // ---------------------
    // Allocate some memory, since the size is not yet known use NLMSG_GOODSIZE
    skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
    if (skb == NULL) {
        printk(KERN_INFO "An error occurred in doc_exmpl_echo:\n");
        return -1;
    }

    // Create the message headers

    // Add header to netlink message;
    // afterwards the buffer looks like this:
    // -------------
    // | netlink header                |
    // | generic netlink header        |
    // | <room for netlink attributes> |
    // ---------------------------------
    msg_head = genlmsg_put(skb, // buffer for netlink message: struct sk_buff *
                           // I thought that (sending) pid must be 0 (the kernel) but this breaks "neli" Rust lib with 
                           // "BadPid" error. I don't know which side is wrong and which follows the standard. We don't have 
                           // any disadvantage in our case if we just set the pid (snd_portid seems to be pid) of the user program.
                           info->snd_portid, // sending pid: int
                           info->snd_seq + 1,  // sequence number: int (might be used by receiver, but not mandatory)
                           &doc_exmpl_gnl_family, // struct genl_family *
                           0, // flags: int (for netlink header)
                           // this way we can trigger a specific command on the receiving side or imply
                           // on which type of command we are currently answering; this is application specific
                           EXMPL_C_ECHO // cmd: u8 (for generic netlink header);
    );
    if (msg_head == NULL) {
        rc = ENOMEM;
        printk(KERN_INFO "An error occurred in doc_exmpl_echo:\n");
        return -rc;
    }
    // Add a EXMPL_A_MSG attribute (actual value/payload to be sent)
    // just echo the value we just received
    rc = nla_put_string(skb, EXMPL_A_MSG, recv_msg);
    if (rc != 0)
    {
        printk(KERN_INFO "An error occurred in doc_exmpl_echo:\n");
        return -rc;
    }

    // Finalize the message:
    // Corrects the netlink message header (length) to include the appended
    // attributes. Only necessary if attributes have been added to the message.
    genlmsg_end(skb, msg_head);

    // Send the message back
    rc = genlmsg_reply(skb, info);
    // same as genlmsg_unicast(genl_info_net(info), skb, info->snd_portid)
    // see https://elixir.bootlin.com/linux/v5.8.9/source/include/net/genetlink.h#L326
    if (rc != 0) {
        printk(KERN_INFO "An error occurred in doc_exmpl_echo:\n");
        return -rc;
    }
    return 0;
}

// An echo command, but we expect the reply to be an error.
// We set nl->nlmsg_type to NLMSG_ERROR
// https://linux.die.net/man/7/netlink
int doc_exmpl_echo_fail(struct sk_buff *skb_2, struct genl_info *info)
{
    struct sk_buff *skb;
    int rc;
    struct nlmsghdr *nlh;
    void * msg_head;

    skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
    if (skb == NULL) {
        printk(KERN_INFO "An error occurred in doc_exmpl_echo_fail:\n");
        return -1;
    }

    // returns pointer to user specific header
    msg_head = genlmsg_put(
           skb, // buffer for netlink message: struct sk_buff *
           info->snd_portid, // sending pid: int
           info->snd_seq + 1,  // sequence number: int
           &doc_exmpl_gnl_family, // struct genl_family *
           0, // flags: int (for netlink header)
           EXMPL_C_ECHO // cmd: u8 (for generic netlink header);
    );
    if (msg_head == NULL) {
        rc = ENOMEM;
        printk(KERN_INFO "An error occurred in doc_exmpl_echo:\n");
        return -rc;
    }
    // get pointer to nl header;
    // minus because of:
    // https://elixir.bootlin.com/linux/v5.8.9/source/net/netlink/genetlink.c#L442
    nlh = msg_head - GENL_HDRLEN - NLMSG_HDRLEN;
    // nlmsg_type is either used for "good message" in this case it is the family number
    // or as "error message", then it's NLMSG_ERROR (0x2)
    printk(KERN_INFO "answering with NLMSG_ERROR for debug reasons\n");
    nlh->nlmsg_type = NLMSG_ERROR;

    // Send the message back
    rc = genlmsg_reply(skb, info);
    // same as genlmsg_unicast(genl_info_net(info), skb, info->snd_portid)
    // see https://elixir.bootlin.com/linux/v5.8.9/source/include/net/genetlink.h#L326
    if (rc != 0) {
        printk(KERN_INFO "An error occurred in doc_exmpl_echo:\n");
        return -rc;
    }
    return 0;
}


static int __init hw_nl_init(void)
{
    int rc;
    printk(KERN_INFO "Generic Netlink Example Module inserted.\n");
    // Register family with it's operations and policies
    rc = genl_register_family(&doc_exmpl_gnl_family);
    if (rc != 0) {
        printk(KERN_INFO "Register ops: %i\n", rc);
        printk(KERN_INFO "An error occurred while inserting the generic netlink example module\n");
        return -1;
    }
    return 0;
}

static void __exit hw_nl_exit(void)
{
    int ret;
    printk(KERN_INFO "Generic Netlink Example Module unloaded.\n");


    // Unregister the family
    ret = genl_unregister_family(&doc_exmpl_gnl_family);
    if (ret != 0) {
        printk(KERN_INFO "Unregister family %i\n", ret);
        return;
    }
}

module_init(hw_nl_init);
module_exit(hw_nl_exit);

MODULE_LICENSE("GPL");
