#pragma once

/* Generic Netlink will create a Netlink family with this name. Kernel will asign
 * a numeric ID and afterwards we can talk to the family with its ID. To get
 * the ID we use Generic Netlink in the userland and pass the family name.
 *
 * Short for: Generic Netlink Foobar Example
 */
#define FAMILY_NAME "gnl_foobar_xmpl"

// Describes common properties/vars/enums of our own protocol
// that sends and receives data on top of generic netlink.

/*
 * These are the attributes that we want to share in gnl_foobar_xmpl.
 * You can understand an attribute as a semantic type. This is
 * the payload of Netlink messages.
 * GNl: Generic Netlink
 */
enum GNlFoobarXmplAttribute {
    GNL_FOOBAR_XMPL_A_UNSPEC, // 0 is never used; first real attribute is 1
    GNL_FOOBAR_XMPL_A_MSG,
    __GNL_FOOBAR_XMPL_A_MAX,
};
/**
 * Max index into enum of type GNlFoobarXmplAttribute.
 */
#define GNL_FOOBAR_XMPL_A_MAX (__GNL_FOOBAR_XMPL_A_MAX - 1)

/*
 * Enumeration of all commands (functions) that our custom protocol on top
 * of generic netlink supports. This can be understood as the action that
 * we want to trigger on the receiving side.
 * GNl: Generic Netlink
 */
enum GNlFoobarXmplCommand {
    GNL_FOOBAR_XMPL_C_UNSPEC, // looks like the 0 entry must always be unused; first real command starts at 1
    // Receives a message (null terminated string) and returns it.
    GNL_FOOBAR_XMPL_C_ECHO,
    __GNL_FOOBAR_XMPL_C_MAX,
};
/**
 * Max index into enum of type GNlFoobarXmplCommand.
 */
#define GNL_FOOBAR_XMPL_C_MAX (__GNL_FOOBAR_XMPL_C_MAX - 1)
