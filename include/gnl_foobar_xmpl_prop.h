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

#pragma once

/*
 * This file describes the common properties of our custom Netlink family on top of Generic Netlink.
 * It is used by all C projects, i.e. the Kernel driver, and the userland components.
 */

/**
 * Generic Netlink will create a Netlink family with this name. Kernel will asign
 * a numeric ID and afterwards we can talk to the family with its ID. To get
 * the ID we use Generic Netlink in the userland and pass the family name.
 *
 * Short for: Generic Netlink Foobar Example
 */
#define FAMILY_NAME "gnl_foobar_xmpl"

/**
 * These are the attributes that we want to share in gnl_foobar_xmpl.
 * You can understand an attribute as a semantic type. This is
 * the payload of Netlink messages.
 * GNl: Generic Netlink
 */
enum GNL_FOOBAR_XMPL_ATTRIBUTE {
    /**
     * 0 is never used (=> UNSPEC), you can also see this in other family definitions in Linux code.
     * We do the same, although I'm not sure, if this is really enforced by code.
     */
    GNL_FOOBAR_XMPL_A_UNSPEC,
    /** We expect a MSG to be a null-terminated C-string. */
    GNL_FOOBAR_XMPL_A_MSG,
    /** Unused marker field to get the length/count of enum entries. No real attribute. */
    __GNL_FOOBAR_XMPL_A_MAX,
};
/**
 * Number of elements in `enum GNL_FOOBAR_XMPL_COMMAND`.
 */
#define GNL_FOOBAR_XMPL_ATTRIBUTE_ENUM_LEN (__GNL_FOOBAR_XMPL_A_MAX)
/**
 * The number of actual usable attributes in `enum GNL_FOOBAR_XMPL_ATTRIBUTE`.
 * This is `GNL_FOOBAR_XMPL_ATTRIBUTE_ENUM_LEN` - 1 because "UNSPEC" is never used.
 */
#define GNL_FOOBAR_XMPL_ATTRIBUTE_COUNT (GNL_FOOBAR_XMPL_ATTRIBUTE_ENUM_LEN - 1)

/**
 * Enumeration of all commands (functions) that our custom protocol on top
 * of generic netlink supports. This can be understood as the action that
 * we want to trigger on the receiving side.
 */
enum GNL_FOOBAR_XMPL_COMMAND {
    /**
     * 0 is never used (=> UNSPEC), you can also see this in other family definitions in Linux code.
     * We do the same, although I'm not sure, if this is really enforced by code.
     */
    GNL_FOOBAR_XMPL_C_UNSPEC,

    // first real command is "1" (>0)
    /**
     * When this command is received, we expect the attribute `GNL_FOOBAR_XMPL_ATTRIBUTE::GNL_FOOBAR_XMPL_A_MSG` to
     * be present in the Generic Netlink request message. The kernel reads the message from the packet and
     * creates a new Generic Netlink response message with an corresponding attribute/payload.
     *
     * This command/signaling mechanism is independent of the Netlink flag `NLM_F_ECHO (0x08)`. We use it as
     * "echo specific data" instead of return a 1:1 copy of the package, which you could do with
     * `NLM_F_ECHO (0x08)` for example.
     */
    GNL_FOOBAR_XMPL_C_ECHO_MSG,

    /**
     * Provokes a NLMSG_ERR answer to this request as described in netlink manpage
     * (https://man7.org/linux/man-pages/man7/netlink.7.html).
     */
    GNL_FOOBAR_XMPL_C_REPLY_WITH_NLMSG_ERR,

    /** Unused marker field to get the length/count of enum entries. No real attribute. */
    __GNL_FOOBAR_XMPL_C_MAX,
};
/**
 * Number of elements in `enum GNL_FOOBAR_XMPL_COMMAND`.
 */
#define GNL_FOOBAR_XMPL_COMMAND_ENUM_LEN (__GNL_FOOBAR_XMPL_C_MAX)
/**
 * The number of actual usable commands  in `enum GNL_FOOBAR_XMPL_COMMAND`.
 * This is `GNL_FOOBAR_XMPL_COMMAND_ENUM_LEN` - 1 because "UNSPEC" is never used.
 */
#define GNL_FOOBAR_XMPL_COMMAND_COUNT (GNL_FOOBAR_XMPL_COMMAND_ENUM_LEN - 1)