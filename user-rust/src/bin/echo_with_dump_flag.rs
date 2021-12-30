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
//! The dump example uses the NLM_F_DUMP flag. A dump can be understand as a
//! "GET ALL DATA OF THE GIVEN ENTITY", i.e. the userland can receive as long as the
//! .dumpit callback returns data. For the sake of simplicity the kernel returns
//! exactly 3 messages (hard coded) during a dump, before the next .dumpit can start.
//! In this example we don't need to send a message that gets echoed back. We get some
//! dummy data that simulates a real world application dump (= give me all your data).
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
};
use std::process;
use user_rust::{FAMILY_NAME, NlFoobarXmplAttribute, NlFoobarXmplCommand};
use neli::consts::nl::Nlmsg;

/// Data we want to send to kernel.
const ECHO_MSG: &str = "Some data that has `Nl` trait implemented, like &str";

fn main() {
    println!("Rust-Binary: echo_with_dump_flag");

    let mut sock = NlSocketHandle::connect(
        NlFamily::Generic,
        // 0 is pid of kernel -> socket is connected to kernel
        Some(0),
        &[],
    ).unwrap();

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

    // in `$ sudo dmesg` you should see that this results in 2 .dumpit runs.
    for _ in 0..2 {
        let nlmsghdr = build_msg(family_id);
        println!("[User-Rust]: Requested DUMP via netlink");

        sock.send(nlmsghdr).expect("Send must work");

        // Because we hard coded into the kernel that we want to receive 3 entities during a dump call
        // we do this 3 times. Why 3? For the sake of simplicity and to show you the basic principle
        // behind it.
        for _ in 0..3 {
            let res: Nlmsghdr<u16, Genlmsghdr<NlFoobarXmplCommand, NlFoobarXmplAttribute>> =
                sock.recv().expect("Should receive a message").unwrap();

            let attr_handle = res.get_payload().unwrap().get_attr_handle();
            let received = attr_handle
                .get_attr_payload_as_with_len::<String>(NlFoobarXmplAttribute::Msg)
                .unwrap();
            println!("[User-Rust]: Received from kernel from .dumpit callback: [seq={}] '{}'", res.nl_seq, received);
        }

        let done_msg: Nlmsghdr<u16, Genlmsghdr<NlFoobarXmplCommand, NlFoobarXmplAttribute>> = sock.recv().expect("Should receive message").unwrap();
        assert_eq!(u16::from(Nlmsg::Done), done_msg.nl_type, "Must receive NLMSG_DONE response" /* 3 is NLMSG_DONE */);
        println!("Received NLMSG_DONE");
    }

}

fn build_msg(family_id: u16) -> Nlmsghdr<u16, Genlmsghdr<NlFoobarXmplCommand, NlFoobarXmplAttribute>> {
    let mut attrs: GenlBuffer<NlFoobarXmplAttribute, Buffer> = GenlBuffer::new();
    attrs.push(
        Nlattr::new(
            false,
            false,
            NlFoobarXmplAttribute::Msg,
            ECHO_MSG,
        )
            .unwrap(),
    );

    // In this DUMP flag example we use the EchoMsg command for the sake of simplicity but
    // we don't actually put a MSG as payload into the request and expect an echo-reply from kernel.

    let gnmsghdr = Genlmsghdr::new(
        NlFoobarXmplCommand::EchoMsg,
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

    nlmsghdr
}
