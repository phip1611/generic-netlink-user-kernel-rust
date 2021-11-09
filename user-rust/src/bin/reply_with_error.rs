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
//!
//! This examples provokes an error response from the kernel.
//! TODO this fails until https://github.com/jbaublitz/neli/issues/116 gets resolved!

use neli::err::NlError;
use neli::{
    consts::{
        nl::{NlmF, NlmFFlags},
        socket::NlFamily,
    },
    genl::{Genlmsghdr, Nlattr},
    nl::{NlPayload, Nlmsghdr},
    socket::NlSocketHandle,
    types::{Buffer, GenlBuffer},
};
use std::process;
use user_rust::{NlFoobarXmplAttribute, NlFoobarXmplCommand, FAMILY_NAME};

fn main() {
    println!("Rust-Binary: reply_with_error");

    let mut sock = NlSocketHandle::connect(
        NlFamily::Generic,
        // 0 is pid of kernel -> socket is connected to kernel
        Some(0),
        &[],
    )
    .unwrap();

    let family_id;
    let res = sock.resolve_genl_family(FAMILY_NAME);
    match res {
        Ok(id) => family_id = id,
        Err(e) => {
            eprintln!(
                "The Netlink family '{}' can't be found. Is the kernel module loaded yet? neli-error='{:#?}'",
                FAMILY_NAME, e
            );
            // exit without error in order for Continuous Integration and automatic testing not to fail
            // when the kernel module is not loaded
            return;
        }
    }

    // some attribute
    let mut attrs: GenlBuffer<NlFoobarXmplAttribute, Buffer> = GenlBuffer::new();
    attrs.push(
        Nlattr::new(
            false,
            false,
            NlFoobarXmplAttribute::Msg,
            "We expect this message to fail",
        )
        .unwrap(),
    );
    let gnmsghdr = Genlmsghdr::new(NlFoobarXmplCommand::ReplyWithNlmsgErr, 1, attrs);
    let nlmsghdr = Nlmsghdr::new(
        None,
        family_id,
        NlmFFlags::new(&[NlmF::Request]),
        None,
        Some(process::id()),
        NlPayload::Payload(gnmsghdr),
    );

    sock.send(nlmsghdr).expect("Send must work");

    let res: Result<
        Option<Nlmsghdr<u16, Genlmsghdr<NlFoobarXmplCommand, NlFoobarXmplAttribute>>>,
        NlError<u16, Genlmsghdr<NlFoobarXmplCommand, NlFoobarXmplAttribute>>,
    > = sock.recv();
    let received_err = res.unwrap_err();
    let received_err = match received_err {
        NlError::Nlmsgerr(err) => err,
        _ => panic!("We expected an error here!"),
    };
    println!(
        "Inside the kernel this error code occurred: {}",
        received_err.error
    );
    let attr_handle = received_err.nlmsg.nl_payload.get_attr_handle();
    println!(
        "ECHO-attribute was: {}",
        attr_handle
            .get_attr_payload_as_with_len::<String>(NlFoobarXmplAttribute::Msg)
            .expect("This attribute was sent and must be in the response")
    );
    println!("NlMsg that caused the error: {:#?}", received_err.nlmsg);

    println!("Everything okay; successfully received error")
}
