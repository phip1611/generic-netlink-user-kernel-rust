#include <linux/module.h>
#include <linux/fs.h>
#include <net/sock.h>
#include <net/genetlink.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>

/* attributes (variables):
 * the index in this enum is used as a reference for the type,
 * userspace application has to indicate the corresponding type
 * the policy is used for security considerations 
 */
enum Attributes {
    DOC_EXMPL_A_UNSPEC,
    DOC_EXMPL_A_MSG,
    __DOC_EXMPL_A_MAX,
};
#define DOC_EXMPL_A_MAX (__DOC_EXMPL_A_MAX - 1)

/* attribute policy: defines which attribute has which type (e.g int, char * etc)
 * possible values defined in net/netlink.h 
 */
static struct nla_policy doc_exmpl_genl_policy[DOC_EXMPL_A_MAX + 1] = {
    [DOC_EXMPL_A_MSG] = {.type = NLA_NUL_STRING},
};

/* commands: enumeration of all commands (functions), 
 * used by userspace application to identify command to be executed
 */
enum Commands {
    DOC_EXMPL_C_UNSPEC, // looks like the 0 entry must always be unused; first real command starts at 1
    DOC_EXMPL_C_ECHO,
    __DOC_EXMPL_C_MAX,
};
#define DOC_EXMPL_C_MAX (__DOC_EXMPL_C_MAX - 1)

// callback function for echo command
int doc_exmpl_echo(struct sk_buff *, struct genl_info *);

// Array with all operations that we support
// An operation is the clue between a command and the
// callback function
struct genl_ops ops[__DOC_EXMPL_C_MAX] = {
    {
        .cmd = DOC_EXMPL_C_ECHO,
        .flags = 0,
        .doit = doc_exmpl_echo,
        .dumpit = NULL,
    }
};

#define VERSION_NR 1
//family definition
static struct genl_family doc_exmpl_gnl_family = {
    .id = 0, // automatically assign an id
    .hdrsize = 0, // we don't use custom additional header info
    .name = "CONTROL_EXMPL", // The name of this family, used by userspace application
    .version = VERSION_NR,   // family specific version number
    .maxattr = DOC_EXMPL_A_MAX, // should also be the bounds check for policy
    .ops = ops, // delegates all incoming requests to callback functions
    .n_ops = DOC_EXMPL_C_MAX,
    .policy = doc_exmpl_genl_policy,
    .module = THIS_MODULE,
};

//An echo command, receives a message, prints it and sends another message back
int doc_exmpl_echo(struct sk_buff *skb_2, struct genl_info *info)
{    
    printk(KERN_INFO "hello-world-nl: doc_exmpl_echo() invoked\n");
    struct nlattr *na;
    struct sk_buff *skb;
    int rc;
    void *msg_head;
    char *mydata;

    if (info == NULL) {
        printk(KERN_INFO "An error occured in doc_exmpl_echo:\n");
        return -1;
    }

    /* For each attribute there is an index in info->attrs which points to a nlattr structure
     * in this structure the data is given
     */
    na = info->attrs[DOC_EXMPL_A_MSG];
    if (na) {
        mydata = (char *)nla_data(na);
        if (mydata == NULL) {
            printk(KERN_INFO "error while receiving data\n");
        }
        else {
            printk(KERN_INFO "received: %s\n", mydata);
        }
    } else {
        printk(KERN_INFO "no info->attrs %i\n", DOC_EXMPL_A_MSG);
    }

    //Send a message back
    //Allocate some memory, since the size is not yet known use NLMSG_GOODSIZE
    skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
    if (skb == NULL) {
        printk(KERN_INFO "An error occured in doc_exmpl_echo:\n");
        return -1;
    }

    //Create the message headers
    /* arguments of genlmsg_put: 
       struct sk_buff *, 
       int (sending) pid, 
       int sequence number, 
       struct genl_family *, 
       int flags, 
       u8 command index (why do we need this?)
    */
    //msg_head = genlmsg_put(skb, 0, info->snd_seq + 1, &doc_exmpl_gnl_family, 0, DOC_EXMPL_C_ECHO);
    msg_head = genlmsg_put(skb, info->snd_portid, info->snd_seq + 1, &doc_exmpl_gnl_family, 0, DOC_EXMPL_C_ECHO);
    if (msg_head == NULL) {
        rc = ENOMEM;
        printk(KERN_INFO "An error occured in doc_exmpl_echo:\n");
        return -rc;
    }
    //Add a DOC_EXMPL_A_MSG attribute (actual value to be sent)
    rc = nla_put_string(skb, DOC_EXMPL_A_MSG, "Hello World from kernel space");
    if (rc != 0)
    {
        printk(KERN_INFO "An error occured in doc_exmpl_echo:\n");
        return -rc;
    }

    //Finalize the message
    genlmsg_end(skb, msg_head);

    //Send the message back
    rc = genlmsg_unicast(genl_info_net(info), skb, info->snd_portid);
    if (rc != 0) {
        printk(KERN_INFO "An error occured in doc_exmpl_echo:\n");
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
        printk(KERN_INFO "An error occured while inserting the generic netlink example module\n");
        return -1;
    }
    return 0;
}

static void __exit hw_nl_exit(void)
{
    int ret;
    printk(KERN_INFO "Generic Netlink Example Module unloaded.\n");


    //Unregister the family
    ret = genl_unregister_family(&doc_exmpl_gnl_family);
    if (ret != 0) {
        printk(KERN_INFO "Unregister family %i\n", ret);
        return;
    }
}

module_init(hw_nl_init);
module_exit(hw_nl_exit);

MODULE_LICENSE("GPL");
