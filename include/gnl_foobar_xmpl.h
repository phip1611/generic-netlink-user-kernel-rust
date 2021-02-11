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
