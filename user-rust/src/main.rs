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

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

//! Userland component written in Rust, that uses neli to talk to a custom Netlink
//! family via Generic Netlink. The family is called "gnl_foobar_xmpl" and the
//! kernel module must be loaded first. Otherwise the family doesn't exist.

use neli::{
    consts::{
        nl::{NlmF, NlmFFlags},
        socket::NlFamily,
    },
    genl::{Genlmsghdr, Nlattr},
    nl::{NlPayload, Nlmsghdr},
    socket::NlSocketHandle,
    types::{Buffer, GenlBuffer},
    utils::U32Bitmask,
};
use std::process;
use std::process::exit;
use std::time::Instant;

/// Name of the Netlink family registered via Generic Netlink
const FAMILY_NAME: &str = "gnl_foobar_xmpl";

/// Data we want to send to kernel.
const ECHO_MSG: &str = "Some data that has `Nl` trait implemented, like &str";

// Implements the necessary trait for the "neli" lib on an enum called "NlFoobarXmplOperation".
// NlFoobarXmplOperation corresponds to "enum GNlFoobarXmplCommand" in kernel module C code.
// Describes what callback function shall be invoked in the linux kernel module.
// This is for the "cmd" field in Generic Netlink header.
neli::impl_var!(
    NlFoobarXmplOperation,
    u8,
    Unspec => 0,
    Echo => 1
);
impl neli::consts::genl::Cmd for NlFoobarXmplOperation {}

// Implements the necessary trait for the "neli" lib on an enum called "NlFoobarXmplAttribute".
// NlFoobarXmplAttribute corresponds to "enum NlFoobarXmplAttribute" in kernel module C code.
// Describes the value type to data mappings inside the generic netlink packet payload.
// This is for the Netlink Attributes (the actual payload) we want to send.
neli::impl_var!(
    NlFoobarXmplAttribute,
    u16,
    Unspec => 0,
    Msg => 1
);
impl neli::consts::genl::NlAttrType for NlFoobarXmplAttribute {}

fn main() {
    let begin = Instant::now();
    for _ in 0..100000 {
        let mut sock = NlSocketHandle::connect(
            NlFamily::Generic,
            // 0 is pid of kernel -> socket is connected to kernel
            Some(0),
            U32Bitmask::empty(),
        )
            .unwrap();

        let family_id;
        let res = sock.resolve_genl_family(FAMILY_NAME);
        match res {
            Ok(id) => family_id = id,
            Err(e) => {
                eprintln!(
                    "The Netlink family '{}' can't be found. Is the kernel module loaded yet? neli-error='{}'",
                    FAMILY_NAME, e
                );
                // exit without error in order for Continuous Integration and automatic testing not to fail
                // when the kernel module is not loaded
                exit(0);
            }
        }

        // println!("[User-Rust]: Generic family number is {}", family_id);

        // We want to send an Echo command
        // 1) prepare NlFoobarXmpl Attribute
        let mut attrs: GenlBuffer<NlFoobarXmplAttribute, Buffer> = GenlBuffer::new();
        attrs.push(
            Nlattr::new(
                None,
                false,
                false,
                // the type of the attribute. This is an u16 and corresponds
                // to an enum on the receiving side
                NlFoobarXmplAttribute::Msg,
                ECHO_MSG,
            )
                .unwrap(),
        );
        // 2) prepare Generic Netlink Header. The Generic Netlink Header contains the
        //    attributes (actual data) as payload.
        let gnmsghdr = Genlmsghdr::new(
            NlFoobarXmplOperation::Echo,
            // You can evolve your application over time using different versions or ignore it.
            // Application specific; receiver can check this value and to specific logic
            1,
            // actual payload
            attrs,
        );
        // 3) Prepare Netlink header. The Netlink header contains the Generic Netlink header
        //    as payload.
        let nlmsghdr = Nlmsghdr::new(
            None,
            family_id,
            // You can use flags in an application specific way (e.g. ACK flag). It is up to you
            // if you check against flags in your Kernel module. It is required to add NLM_F_REQUEST,
            // otherwise the Kernel doesn't route the packet to the right Netlink callback handler
            // in your Kernel module. This might result in a deadlock on the socket if an expected
            // reply you are waiting for in a blocking way is never received.
            // Kernel reference: https://elixir.bootlin.com/linux/v5.10.16/source/net/netlink/af_netlink.c#L2487
            NlmFFlags::new(&[NlmF::Request]),
            // It is up to you if you want to split a data transfer into multiple sequences. (application specific)
            None,
            // Port ID. Not necessarily the process id of the current process. This field
            // could be used to identify different points or threads inside your application
            // that send data to the kernel. This has nothing to do with "routing" the packet to
            // the kernel, because this is done by the socket itself
            Some(process::id()),
            NlPayload::Payload(gnmsghdr),
        );

        // println!("[User-Rust]: Sending '{}' via netlink", ECHO_MSG);

        // Send data
        sock.send(nlmsghdr).expect("Send must work");

        // receive echo'ed message
        let res: Nlmsghdr<u16, Genlmsghdr<NlFoobarXmplOperation, NlFoobarXmplAttribute>> =
            sock.recv().expect("Should receive a message").unwrap();

        /* USELESS, just note: this is always the case. Otherwise neli would have returned Error
        if res.nl_type == family_id {
            println!("Received successful reply!");
        }*/

        let attr_handle = res.get_payload().unwrap().get_attr_handle();
        let received = attr_handle
            .get_attr_payload_as::<String>(NlFoobarXmplAttribute::Msg)
            .unwrap();
        // println!("[User-Rust]: Received from kernel: '{}'", received);
    }
    let us = begin.elapsed().as_micros() / 100000;
    println!("took {}µs on average", us);
}
