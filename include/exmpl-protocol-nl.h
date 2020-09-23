#pragma once

#define FAMILY_NAME "CONTROL_EXMPL"

// Describes common properties/vars/enums of our own protocol
// that sends and receives data on top of generic netlink.

/*
 * The attributes that we send on top of generic netlink.
 * This is the kind of payload we can send per message.
 */
enum Attribute {
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
enum Command {
    EXMPL_C_UNSPEC, // looks like the 0 entry must always be unused; first real command starts at 1
    // Receives a message (null terminated string) and returns it.
    EXMPL_C_ECHO,
    // Like EXMPL_C_ECHO but we receive an error reply. This way we can test how to cope with errors.
    EXMPL_C_ECHO_FAIL,
    __EXMPL_C_MAX,
};
#define EXMPL_C_MAX (__EXMPL_C_MAX - 1)
