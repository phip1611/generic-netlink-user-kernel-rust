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

//! The fully documented binary is `src/bin/echo.rs`. In this file only changes, according
//! to the description of the filename, are documented.
//!
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
use user_rust::{FAMILY_NAME, NlFoobarXmplAttribute, NlFoobarXmplOperation};

/// Data we want to send to kernel.
const ECHO_MSG: &str = "Some data that has `Nl` trait implemented, like &str";

fn main() {
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
            return;
        }
    }

    println!("[User-Rust]: Generic family number is {}", family_id);

    let mut attrs: GenlBuffer<NlFoobarXmplAttribute, Buffer> = GenlBuffer::new();
    attrs.push(
        Nlattr::new(
            None,
            false,
            false,
            NlFoobarXmplAttribute::Msg,
            ECHO_MSG,
        )
            .unwrap(),
    );

    let gnmsghdr = Genlmsghdr::new(
        NlFoobarXmplOperation::Echo,
        1,
        attrs,
    );

    let nlmsghdr = Nlmsghdr::new(
        None,
        family_id,
        // This time we are sending the Dump-Flag which will invoke the
        // corresponding .dumpit callback in the kernel
        NlmFFlags::new(&[NlmF::Request, NlmF::Dump]),
        None,
        Some(process::id()),
        NlPayload::Payload(gnmsghdr),
    );

    println!("[User-Rust]: Sending '{}' via netlink", ECHO_MSG);

    sock.send(nlmsghdr).expect("Send must work");

    let res: Nlmsghdr<u16, Genlmsghdr<NlFoobarXmplOperation, NlFoobarXmplAttribute>> =
        sock.recv().expect("Should receive a message").unwrap();

    let attr_handle = res.get_payload().unwrap().get_attr_handle();
    let received = attr_handle
        .get_attr_payload_as::<String>(NlFoobarXmplAttribute::Msg)
        .unwrap();
    println!("[User-Rust]: Received from kernel: '{}'", received);
}