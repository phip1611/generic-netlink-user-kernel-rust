#pragma once

// Describes common properties/vars/enums of our own protocol
// that sends and receives data on top of generic netlink.

/*
 * The attributes that we send on top of generic netlink.
 * This is the kind of payload we can send per message.
 */
enum Attributes {
    EXMPL_A_UNSPEC, // 0 is never used; first real attribute is 1
    EXMPL_A_MSG,
    __EXMPL_A_MAX,
};
#define EXMPL_A_MAX (__EXMPL_A_MAX - 1)

/*
 * Enumeration of all commands (functions) that our custom protocol on top
 * of generic netlink supports. This can be understood as the action that
 * we want to trigger on the receiving side.
 */
enum Commands {
    EXMPL_C_UNSPEC, // looks like the 0 entry must always be unused; first real command starts at 1
    EXMPL_C_ECHO,
    __EXMPL_C_MAX,
};
#define EXMPL_C_MAX (__EXMPL_C_MAX - 1)