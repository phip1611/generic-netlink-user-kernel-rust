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

#include <linux/module.h>
#include <net/sock.h>
#include <net/genetlink.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>

// data/vars/enums/properties that describes our protocol that we implement
// on top of generic netlink (like functions we want to trigger on the receiving side)
#include "gnl_foobar_xmpl.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Philipp Schuster <phip1611@gmail.com>");
MODULE_DESCRIPTION(
    "Linux driver that registers the Netlink family "
    "\"gnl_foobar_xmpl\" via Generic Netlink and responds to echo messages."
);

/* attribute policy: defines which attribute has which type (e.g int, char * etc)
 * possible values defined in net/netlink.h 
 */
// TODO where/how is this actually validated?
static struct nla_policy gnl_foobar_xmpl_policy[GNL_FOOBAR_XMPL_A_MAX + 1] = {
    [GNL_FOOBAR_XMPL_A_MSG] = {.type = NLA_NUL_STRING},
};

// callback function for echo command
int gnl_foobar_xmpl_cb_echo(struct sk_buff *sender_skb, struct genl_info *info);

// Array with all operations that out protocol on top of generic netlink
// supports. An operation is the glue between a command (number) and the
// corresponding callback function.
struct genl_ops gnl_foobar_xmpl_ops[GNL_FOOBAR_XMPL_C_MAX + 1] = {
    {
        .cmd = GNL_FOOBAR_XMPL_C_ECHO,
        .flags = 0,
        .doit = gnl_foobar_xmpl_cb_echo,
        .dumpit = NULL,
    }
};

// family definition
static struct genl_family gnl_foobar_xmpl_family = {
    .id = 0, // automatically assign an id
    .hdrsize = 0, // we don't use custom additional header info
    .name = FAMILY_NAME, // The name of this family, used by userspace application to get the numeric ID
    .version = 1,   // family specific version number; can be used to evole application over time (multiple versions)
    .maxattr = GNL_FOOBAR_XMPL_A_MAX, // should also be the bounds check for policy
    .ops = gnl_foobar_xmpl_ops, // delegates all incoming requests to callback functions
    .n_ops = GNL_FOOBAR_XMPL_C_MAX,
    .policy = gnl_foobar_xmpl_policy,
    .module = THIS_MODULE,
};

// Callback function if a message with command GNL_FOOBAR_XMPL_C_ECHO was received
int gnl_foobar_xmpl_cb_echo(struct sk_buff *_unused_sender_skb, struct genl_info *info)
{    
    struct nlattr *na;
    struct sk_buff *reply_skb;
    int rc;
    void * msg_head;
    char * recv_msg;

    printk(KERN_INFO "generic-netlink-demo-km: %s() invoked\n", __func__);

    if (info == NULL) {
        // should never happen
        printk(KERN_INFO "An error occurred in %s():\n", __func__);
        return -1;
    }

    /* For each attribute there is an index in info->attrs which points to a nlattr structure
     * in this structure the data is given
     */
    na = info->attrs[GNL_FOOBAR_XMPL_A_MSG];
    if (na) {
        recv_msg = (char *)nla_data(na);
        if (recv_msg == NULL) {
            printk(KERN_INFO "error while receiving data\n");
        }
        else {
            printk(KERN_INFO "received: '%s'\n", recv_msg);
        }
    } else {
        printk(KERN_INFO "no info->attrs[%i]\n", GNL_FOOBAR_XMPL_A_MSG);
        return -1; // we return here because we expect to recv a msg
    }

    // Send a message back
    // ---------------------
    // Allocate some memory, since the size is not yet known use NLMSG_GOODSIZE
    reply_skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
    if (reply_skb == NULL) {
        printk(KERN_INFO "An error occurred in %s():\n", __func__);
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
    msg_head = genlmsg_put(reply_skb, // buffer for netlink message: struct sk_buff *
                           // According to my findings: this is not used for routing
                           // This can be used in an application specific way to target
                           // different endpoints within the same user application
                           // but general rule: just put sender port id here
                           info->snd_portid, // sending port (not process) id: int
                           info->snd_seq + 1,  // sequence number: int (might be used by receiver, but not mandatory)
                           &gnl_foobar_xmpl_family, // struct genl_family *
                           0, // flags: int (for netlink header); we don't check them in the userland; application specific
                           // this way we can trigger a specific command/callback on the receiving side or imply
                           // on which type of command we are currently answering; this is application specific
                           GNL_FOOBAR_XMPL_C_ECHO // cmd: u8 (for generic netlink header);
    );
    if (msg_head == NULL) {
        rc = ENOMEM;
        printk(KERN_INFO "An error occurred in %s():\n", __func__);
        return -rc;
    }
    // Add a GNL_FOOBAR_XMPL_A_MSG attribute (actual value/payload to be sent)
    // just echo the value we just received
    rc = nla_put_string(reply_skb, GNL_FOOBAR_XMPL_A_MSG, recv_msg);
    if (rc != 0)
    {
        printk(KERN_INFO "An error occurred in %s():\n", __func__);
        return -rc;
    }

    // Finalize the message:
    // Corrects the netlink message header (length) to include the appended
    // attributes. Only necessary if attributes have been added to the message.
    genlmsg_end(reply_skb, msg_head);

    // Send the message back
    rc = genlmsg_reply(reply_skb, info);
    // same as genlmsg_unicast(genl_info_net(info), reply_skb, info->snd_portid)
    // see https://elixir.bootlin.com/linux/v5.8.9/source/include/net/genetlink.h#L326
    if (rc != 0) {
        printk(KERN_INFO "An error occurred in %s():\n", __func__);
        return -rc;
    }
    return 0;
}


static int __init gnl_foobar_xmpl_module_init(void)
{
    int rc;
    printk(KERN_INFO "Generic Netlink Example Module inserted.\n");
    // Register family with it's operations and policies
    rc = genl_register_family(&gnl_foobar_xmpl_family);
    if (rc != 0) {
        printk(KERN_INFO "Register gnl_foobar_xmpl_ops: %i\n", rc);
        printk(KERN_INFO "An error occurred while inserting the generic netlink example module\n");
        return -1;
    }
    return 0;
}

static void __exit gnl_foobar_xmpl_module_exit(void)
{
    int ret;
    printk(KERN_INFO "Generic Netlink Example Module unloaded.\n");

    // Unregister the family
    ret = genl_unregister_family(&gnl_foobar_xmpl_family);
    if (ret != 0) {
        printk(KERN_INFO "Unregister family %i\n", ret);
        return;
    }
}

module_init(gnl_foobar_xmpl_module_init);
module_exit(gnl_foobar_xmpl_module_exit);

