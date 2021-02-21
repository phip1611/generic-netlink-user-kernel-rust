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
// ##################################################################################################
/*
 * This is a Linux kernel module/driver called "gnl_foobar_xmpl" which shows you the basics of using
 * Generic Netlink in the kernel. It registers a Netlink family called "gnl_foobar_xmpl". See
 * "gnl_foobar_xmpl_prop.h" for common properties of the family. "Generic Netlink" offers us a lot of
 * convenient functions to register new/custom Netlink families on the fly during runtime. We use this
 * functionality here. It implements simple IPC between Userland and Kernel (Kernel responds to userland).
 *
 * Check "gnl_foobar_xmpl_prop.h" for common properties of the family first, afterwards follow the code here.
 *
 * You can find some more interesting documentation about Generic Netlink here:
 * "Generic Netlink HOW-TO based on Jamal's original doc" https://lwn.net/Articles/208755/
 */

// basic definitions for kernel module development
#include <linux/module.h>
// definitions for generic netlink families, policies etc;
// transitive dependencies for basic netlink, sockets etc
#include <net/genetlink.h>
// required for locking inside the .dumpit callback demonstration
#include <linux/mutex.h>

// data/vars/enums/properties that describes our protocol that we implement
// on top of generic netlink (like functions we want to trigger on the receiving side)
#include "gnl_foobar_xmpl_prop.h"

// Module/Driver description.
// You can see this for example when executing `$ modinfo ./gnl_foobar_xmpl.ko` (after build).
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Philipp Schuster <phip1611@gmail.com>");
MODULE_DESCRIPTION(
        "Linux driver that registers the custom Netlink family "
        "\"" FAMILY_NAME "\" via Generic Netlink and responds to echo messages "
                         "according to the properties specified in \"gnl_foobar_xmpl_prop.h\"."
);


/* ######################## CONVENIENT LOGGING MACROS ######################## */
// (Re)definition of some convenient logging macros from <linux/printk.h>. You can see the logging
// messages when printing the kernel log, e.g. with `$ sudo dmesg`.
// See https://elixir.bootlin.com/linux/latest/source/include/linux/printk.h

// with this redefinition we can easily prefix all log messages from pr_* logging macros
#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
/* ########################################################################### */

/**
 * Data structure required for our .dumpit callback handler to
 * know about the progress of an ongoing dump.
 * See the dumpit callback handler how it is used.
 */
struct {
    // from <linux/mutex.h>
    /**
     * Only one process is allowed per dump process. We need a lock for that.
     */
    struct mutex mtx;
    /**
     * Number that describes how many packets we need to send until we are done
     * during an ongoing dumpit process. 0 = done.
     */
    int unsigned runs_to_go;
    /**
     * Number that describes how many packets per dump are sent in total.
     * Constant per dump.
     */
    int unsigned total_runs;
} dumpit_cb_progress_data;

// Documentation is on the implementation of this function.
int gnl_cb_doit_echo(struct sk_buff *sender_skb, struct genl_info *info);

// Documentation is on the implementation of this function.
int gnl_cb_dumpit_generic(struct sk_buff *pre_allocated_skb, struct netlink_callback *cb);

// Documentation is on the implementation of this function.
int	gnl_cb_dumpit_before_generic(struct netlink_callback *cb);

// Documentation is on the implementation of this function.
int	gnl_cb_dumpit_after_generic(struct netlink_callback *cb);

// Documentation is on the implementation of this function.
int gnl_cb_doit_reply_with_nlmsg_err(struct sk_buff *sender_skb, struct genl_info *info);

/**
 * The length of `struct genl_ops gnl_foobar_xmpl_ops[]`. Not necessarily
 * the number of commands in `enum GNlFoobarXmplCommand`. It depends on your application logic.
 * For example, you can use the same command multiple times and - dependent by flag -
 * invoke a different callback handler. In our simple example we just use one .doit callback
 * per operation/command.
 */
#define GNL_FOOBAR_OPS_LEN (GNL_FOOBAR_XMPL_COMMAND_COUNT)

/**
 * Array with all operations that our protocol on top of Generic Netlink
 * supports. An operation is the glue between a command ("cmd" field in `struct genlmsghdr` of
 * received Generic Netlink message) and the corresponding ".doit" callback function.
 * See: https://elixir.bootlin.com/linux/v5.11/source/include/net/genetlink.h#L148
 */
struct genl_ops gnl_foobar_xmpl_ops[GNL_FOOBAR_OPS_LEN] = {
        {
                /* The "cmd" field in `struct genlmsghdr` of received Generic Netlink message */
                .cmd = GNL_FOOBAR_XMPL_C_ECHO,
                /* TODO Use case ? */
                .flags = 0,
                /* TODO Use case ? */
                .internal_flags = 0,
                /* Callback handler when a request with the specified ".cmd" above is received.
                 * Always validates the payload except one set NO_STRICT_VALIDATION flag in ".validate"
                 * See: https://elixir.bootlin.com/linux/v5.11/source/net/netlink/genetlink.c#L717
                 *
                 * Quote from: https://lwn.net/Articles/208755
                 *  "The 'doit' handler should do whatever processing is necessary and return
                 *   zero on success, or a negative value on failure.  Negative return values
                 *   will cause a NLMSG_ERROR message to be sent while a zero return value will
                 *   only cause a NLMSG_ERROR message to be sent if the request is received with
                 *   the NLM_F_ACK flag set."
                 *
                 * You can find this in Linux code here:
                 * https://elixir.bootlin.com/linux/v5.11/source/net/netlink/af_netlink.c#L2499
                 *
                 * One can find more information about NLMSG_ERROR responses and how to handle them
                 * in userland in the manpage: https://man7.org/linux/man-pages/man7/netlink.7.html
                 *
                 */
                .doit = gnl_cb_doit_echo,
                /* This callback is similar in use to the standard Netlink 'dumpit' callback.
                 * The 'dumpit' callback is invoked when a Generic Netlink message is received
                 * with the NLM_F_DUMP flag set.
                 *
                 * A dump can be understand as a "GET ALL DATA OF THE GIVEN ENTITY", i.e.
                 * the userland can receive as long as the .dumpit callback returns data.
                 *
                 * .dumpit is not mandatory, but either it or .doit must be provided, see
                 * https://elixir.bootlin.com/linux/v5.11/source/net/netlink/genetlink.c#L367
                 *
                 * To be honest I don't know in what use case one should use .dumpit and why
                 * it is useful, because you can achieve the same also with .doit handlers.
                 * Anyway, this is just an example/tutorial.
                 *
                 * Quote from: https://lwn.net/Articles/208755
                 *  "The main difference between a 'dumpit' handler and a 'doit' handler is
                 *   that a 'dumpit' handler does not allocate a message buffer for a response;
                 *   a pre-allocated sk_buff is passed to the 'dumpit' handler as the first
                 *   parameter.  The 'dumpit' handler should fill the message buffer with the
                 *   appropriate response message and return the size of the sk_buff,
                 *   i.e. sk_buff->len, and the message buffer will automatically be sent to the
                 *   Generic Netlink client that initiated the request.  As long as the 'dumpit'
                 *   handler returns a value greater than zero it will be called again with a
                 *   newly allocated message buffer to fill, when the handler has no more data
                 *   to send it should return zero; error conditions are indicated by returning
                 *   a negative value.  If necessary, state can be preserved in the
                 *   netlink_callback parameter which is passed to the 'dumpit' handler; the
                 *   netlink_callback parameter values will be preserved across handler calls
                 *   for a single request."
                 *
                 * You can see the check for the NLM_F_DUMP-flag here:
                 * https://elixir.bootlin.com/linux/v5.11/source/net/netlink/genetlink.c#L780
                 */
                .dumpit = gnl_cb_dumpit_generic,
                /* Start callback for dumps. Can be used to lock data structures. */
                .start = gnl_cb_dumpit_before_generic,
                /* Completion callback for dumps. Can be used for cleanup after a dump and releasing locks. */
                .done = gnl_cb_dumpit_after_generic,
                /*
                 0 (= "validate strictly") or value `enum genl_validate_flags`
                 * see: https://elixir.bootlin.com/linux/v5.11/source/include/net/genetlink.h#L108
                 */
                .validate = 0,
        },
        {
                .cmd = GNL_FOOBAR_XMPL_C_REPLY_WITH_NLMSG_ERR,
                .flags = 0,
                .internal_flags = 0,
                .doit = gnl_cb_doit_reply_with_nlmsg_err,
                // in a real application you probably have different .dumpit handlers per operation/command
                .dumpit = gnl_cb_dumpit_generic,
                // in a real application you probably have different .start handlers per operation/command
                .start = gnl_cb_dumpit_before_generic,
                // in a real application you probably have different .done handlers per operation/command
                .done = gnl_cb_dumpit_after_generic,
                .validate = 0,
        }
};

/**
 * Attribute policy: defines which attribute has which type (e.g int, char * etc).
 * This get validated for each received Generic Netlink message, if not deactivated
 * in `gnl_foobar_xmpl_ops[].validate`.
 * See https://elixir.bootlin.com/linux/v5.11/source/net/netlink/genetlink.c#L717
 */
static struct nla_policy gnl_foobar_xmpl_policy[GNL_FOOBAR_XMPL_ATTRIBUTE_ENUM_LEN] = {
        // In case you are seeing this syntax for the first time (I also learned this just after a few years of
        // coding with C myself): The following static array initiations are equivalent:
        // `int a[2] = {1, 2}` <==> `int a[2] = {[0] => 1, [1] => 2}`.

        [GNL_FOOBAR_XMPL_A_UNSPEC] = {.type = NLA_UNSPEC},

        // You can set this to NLA_U32 for testing and send an ECHO message from the userland
        // It will fail in this case and you see a entry in the kernel log.

        // `enum GNL_FOOBAR_XMPL_ATTRIBUTE::GNL_FOOBAR_XMPL_A_MSG` is a null-terminated C-String
        [GNL_FOOBAR_XMPL_A_MSG] = {.type = NLA_NUL_STRING},
};

/**
 * Definition of the Netlink family we want to register using Generic Netlink functionality
 */
static struct genl_family gnl_foobar_xmpl_family = {
        // automatically assign an id
        .id = 0,
        // we don't use custom additional header info / user specific header
        .hdrsize = 0,
        // The name of this family, used by userspace application to get the numeric ID
        .name = FAMILY_NAME,
        // family specific version number; can be used to evolve application over time (multiple versions)
        .version = 1,
        // delegates all incoming requests to callback functions
        .ops = gnl_foobar_xmpl_ops,
        // length of array `gnl_foobar_xmpl_ops`
        .n_ops = GNL_FOOBAR_OPS_LEN,
        // attribute policy (for validation of messages). Enforced automatically, except ".validate" in
        // corresponding ".ops"-field is set accordingly.
        .policy = gnl_foobar_xmpl_policy,
        // Number of attributes / bounds check for policy (array length)
        .maxattr = GNL_FOOBAR_XMPL_ATTRIBUTE_ENUM_LEN,
        // Owning Kernel module of the Netlink family we register.
        .module = THIS_MODULE,

        // Actually not necessary because this memory region would be zeroed anyway during module load,
        // but this way one sees all possible options.

        // if your application must handle multiple netlink calls in parallel (where one should not block the next
        // from starting), set this to true! otherwise all netlink calls are mutually exclusive
        .parallel_ops = 0,
        // set to true if the family can handle network namespaces and should be presented in all of them
        .netnsok = 0,
        // called before an operation's doit callback, it may do additional, common, filtering and return an error
        .pre_doit = NULL,
        // called after an operation's doit callback, it may undo operations done by pre_doit, for example release locks
        .post_doit = NULL,
};

/**
 * Regular ".doit"-callback function if a Generic Netlink with command `GNL_FOOBAR_XMPL_C_ECHO` is received.
 * Please look into the comments where this is used as ".doit" callback above in
 * `struct genl_ops gnl_foobar_xmpl_ops[]` for more information about ".doit" callbacks.
*/
int gnl_cb_doit_echo(struct sk_buff *sender_skb, struct genl_info *info) {
    struct nlattr *na;
    struct sk_buff *reply_skb;
    int rc;
    void *msg_head;
    char *recv_msg;

    pr_info("generic-netlink-demo-km: %s() invoked\n", __func__);

    if (info == NULL) {
        // should never happen
        pr_err("An error occurred in %s():\n", __func__);
        return -EINVAL;
    }

    /*
     * For each attribute there is an index in info->attrs which points to a nlattr structure
     * in this structure the data is stored.
     */
    na = info->attrs[GNL_FOOBAR_XMPL_A_MSG];

    if (!na) {
        pr_err("no info->attrs[%i]\n", GNL_FOOBAR_XMPL_A_MSG);
        return -EINVAL; // we return here because we expect to recv a msg
    }

    recv_msg = (char *) nla_data(na);
    if (recv_msg == NULL) {
        pr_err("error while receiving data\n");
    } else {
        pr_info("received: '%s'\n", recv_msg);
    }


    // Send a message back
    // ---------------------

    // Allocate some memory, since the size is not yet known use NLMSG_GOODSIZE
    reply_skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
    if (reply_skb == NULL) {
        pr_err("An error occurred in %s():\n", __func__);
        return -ENOMEM;
    }

    // Create the message headers

    // Add header to netlink message;
    // afterwards the buffer looks like this:
    // ----------------------------------
    // | netlink header                 |
    // | generic netlink header         |
    // | <space for netlink attributes> |
    // ----------------------------------
    msg_head = genlmsg_put(reply_skb, // buffer for netlink message: struct sk_buff *
                           // According to my findings: this is not used for routing
                           // This can be used in an application specific way to target
                           // different endpoints within the same user application
                           // but general rule: just put sender port id here
                           info->snd_portid, // sending port (not process) id: int
                           info->snd_seq + 1, // sequence number: int (might be used by receiver, but not mandatory)
                           &gnl_foobar_xmpl_family, // struct genl_family *
                           0, // flags for Netlink header: int; application specific and not mandatory
                           // The command/operation (u8) from `enum GNL_FOOBAR_XMPL_COMMAND` for Generic Netlink header
                           GNL_FOOBAR_XMPL_C_ECHO
    );
    if (msg_head == NULL) {
        rc = ENOMEM;
        pr_err("An error occurred in %s():\n", __func__);
        return -rc;
    }

    // Add a GNL_FOOBAR_XMPL_A_MSG attribute (actual value/payload to be sent)
    // echo the value we just received
    rc = nla_put_string(reply_skb, GNL_FOOBAR_XMPL_A_MSG, recv_msg);
    if (rc != 0) {
        pr_err("An error occurred in %s():\n", __func__);
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
        pr_err("An error occurred in %s():\n", __func__);
        return -rc;
    }
    return 0;
}

/**
 * Generic ".dumpit"-callback function if a Generic Netlink with flag `NLM_F_DUMP` is received.
 * Please look into the comments where this is used as "..dumpit" callback above in
 * `struct genl_ops gnl_foobar_xmpl_ops[]` for more information about ".dumpit" callbacks.
*/
int gnl_cb_dumpit_generic(struct sk_buff *pre_allocated_skb, struct netlink_callback *cb) {
    void *msg_head;
    int ret;
    static const char HELLO_FROM_DUMPIT_MSG[] = "You set the flag NLM_F_DUMP; this message is "
                                                "brought to you by .dumpit callback :)";
    pr_info("Called %s()\n", __func__);

    if (dumpit_cb_progress_data.runs_to_go == 0) {
        pr_info("no more data to send in dumpit cb\n");
        // mark that dump is done;
        return 0;
    } else {
        dumpit_cb_progress_data.runs_to_go--;
        pr_info("%s: %d more runs to do\n", __func__, dumpit_cb_progress_data.runs_to_go);
    }

    msg_head = genlmsg_put(pre_allocated_skb, // buffer for netlink message: struct sk_buff *
            // According to my findings: this is not used for routing
            // This can be used in an application specific way to target
            // different endpoints within the same user application
            // but general rule: just put sender port id here
                           cb->nlh->nlmsg_pid, // sending port (not process) id: int
            // sequence number: int (might be used by receiver, but not mandatory)
            // sequence 0, 1, 2...
                           dumpit_cb_progress_data.total_runs - dumpit_cb_progress_data.runs_to_go - 1,
                           &gnl_foobar_xmpl_family, // struct genl_family *
                           0, // flags: int (for netlink header); we don't check them in the userland; application specific
            // this way we can trigger a specific command/callback on the receiving side or imply
            // on which type of command we are currently answering; this is application specific
                           GNL_FOOBAR_XMPL_C_ECHO // cmd: u8 (for generic netlink header);
    );
    if (msg_head == NULL) {
        pr_info("An error occurred in %s(): genlmsg_put() failed\n", __func__);
        return -ENOMEM;
    }
    ret = nla_put_string(
            pre_allocated_skb,
            GNL_FOOBAR_XMPL_A_MSG,
            HELLO_FROM_DUMPIT_MSG
    );
    if (ret < 0) {
        pr_info("An error occurred in %s():\n", __func__);
        return ret;
    }
    genlmsg_end(pre_allocated_skb, msg_head);

    // return the length of data we wrote into the pre-allocated buffer
    return pre_allocated_skb->len;
}

/**
 * Regular ".doit"-callback function if a Generic Netlink with command `GNL_FOOBAR_XMPL_C_REPLY_WITH_NLMSG_ERR` is received.
 * Please look into the comments where this is used as ".doit" callback above in
 * `struct genl_ops gnl_foobar_xmpl_ops[]` for more information about ".doit" callbacks.
*/
int gnl_cb_doit_reply_with_nlmsg_err(struct sk_buff *sender_skb, struct genl_info *info) {
    pr_info("generic-netlink-demo-km: %s() invoked\n", __func__);
    pr_info("flags: %x\n", info->nlhdr->nlmsg_flags);

    /*
     * Generic Netlink is smart enough and sends a NLMSG_ERR reply automatically as reply
     * Quote from https://lwn.net/Articles/208755/:
     *  "The 'doit' handler should do whatever processing is necessary and return
     *   zero on success, or a negative value on failure.  Negative return values
     *   will cause a NLMSG_ERROR message to be sent while a zero return value will
     *   only cause a NLMSG_ERROR message to be sent if the request is received with
     *   the NLM_F_ACK flag set."
     *
     * You can find this in Linux code here:
     * https://elixir.bootlin.com/linux/v5.11/source/net/netlink/af_netlink.c#L2499
     *
     * One can find more information about NLMSG_ERROR responses and how to handle them
     * in userland in the manpage: https://man7.org/linux/man-pages/man7/netlink.7.html
     */
    return -EINVAL;
}

int	gnl_cb_dumpit_before_generic(struct netlink_callback *cb) {
    int ret;
    static int unsigned const dump_runs = 3;
    pr_info("%s: dump started. acquire lock. initialize dump runs_to_go (number of receives userland can make) to %d runs\n", __func__, dump_runs);
    // Lock the mutex like mutex_lock(), and return 0 if the mutex has been acquired or sleep until the mutex becomes available
    // If a signal arrives while waiting for the lock then this function returns -EINTR.
    ret = mutex_lock_interruptible(&dumpit_cb_progress_data.mtx);
    if (ret != 0) {
        pr_err("Failed to get lock!\n");
        return ret;
    }
    dumpit_cb_progress_data.total_runs = dump_runs;
    dumpit_cb_progress_data.runs_to_go = dump_runs;
    return 0;

}

// Documentation is on the implementation of this function.
int	gnl_cb_dumpit_after_generic(struct netlink_callback *cb) {
    pr_info("%s: dump done. release lock\n", __func__);
    mutex_unlock(&dumpit_cb_progress_data.mtx);
    return 0;
}

static int __init gnl_foobar_xmpl_module_init(void) {
    int rc;
    pr_info("Generic Netlink Example Module inserted.\n");

    // Register family with its operations and policies
    rc = genl_register_family(&gnl_foobar_xmpl_family);
    if (rc != 0) {
        pr_err("FAILED: genl_register_family(): %i\n", rc);
        pr_err("An error occurred while inserting the generic netlink example module\n");
        return -1;
    } else {
        pr_info("successfully registered custom Netlink family '" FAMILY_NAME "' using Generic Netlink.\n");
    }

    mutex_init(&dumpit_cb_progress_data.mtx);

    return 0;
}

static void __exit gnl_foobar_xmpl_module_exit(void) {
    int ret;
    pr_info("Generic Netlink Example Module unloaded.\n");

    // Unregister the family
    ret = genl_unregister_family(&gnl_foobar_xmpl_family);
    if (ret != 0) {
        pr_err("genl_unregister_family() failed: %i\n", ret);
        return;
    } else {
        pr_info("successfully unregistered custom Netlink family '" FAMILY_NAME "' using Generic Netlink.\n");
    }

    mutex_destroy(&dumpit_cb_progress_data.mtx);
}

module_init(gnl_foobar_xmpl_module_init);
module_exit(gnl_foobar_xmpl_module_exit);
