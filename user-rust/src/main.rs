//! Make sure to make familiar with netlink and generic netlink first!
//!
//! This code is the user part written in Rust that talks via a generic netlink family called
//! "CONTROL_EXMPL" to a linux kernel module. It uses the "neli" lib/crate.
//!
//! Imagine that you are developing an application or a software with multiple components
//! (at least userland program and kernel module) that has several operations you want
//! to trigger on the receiving side. These are "operations" or "commands" and referenced
//! by an u8 (basically an enum) on the receiving side. Further you can specify
//! attributes, the actual data you want to transfer.
//!
//! Our application here is called "Foobar", has "FoobarOperations" and "FoobarAttributes".
//!
//! Each netlink packet has a generic netlink packet as it's payload. In the generic netlink
//! package one can define several attributes (type to value mappings).
//!
//! Workflow is the following:
//! 0) kernel module registers generic netlink family by name which creates a new
//!    virtual/temporary netlink family (a number)
//! 1) client request family id of "CONTROL_EXMPL" (ONCE during application startup)
//! 2) client sends data to retrieved family id. (Kernel module must be able to process it)
//! 3) client can get replies

use neli::consts::nl::{NlmF, NlmFFlags};
use neli::consts::socket::NlFamily;
use neli::genl::{Genlmsghdr, Nlattr};
use neli::nl::{NlPayload, Nlmsghdr};
use neli::socket::NlSocketHandle;
use neli::types::{Buffer, GenlBuffer};
use neli::utils::U32Bitmask;
use neli::Nl;
use std::process;

// Implements the necessary trait for the "neli" lib on an enum called "FoobarOperation".
// FoobarOperation corresponds to "enum Commands" in kernel module C code.
// Describes what callback function shall be invoked in the linux kernel module.
// This is for the "cmd" field in Generic Netlink header.
neli::impl_var!(
    FoobarOperation,
    u8,
    Unspec => 0,
    Echo => 1,
    // we expect an error reply for this command
    EchoFail => 2
);
impl neli::consts::genl::Cmd for FoobarOperation {}

// Implements the necessary trait for the "neli" lib on an enum called "FoobarAttribute".
// FoobarAttribute corresponds to "enum Attributes" in kernel module C code.
// Describes the value type to data mappings inside the generic netlink packet payload.
// This is for the Netlink Attributes (the actual payload) we want to send.
neli::impl_var!(
    FoobarAttribute,
    u16,
    Unspec => 0,
    Msg => 1
);
impl neli::consts::genl::NlAttrType for FoobarAttribute {}

fn main() {
    let mut sock = NlSocketHandle::connect(
        NlFamily::Generic,
        // 0 is pid of kernel -> socket is connected to kernel
        Some(0),
        U32Bitmask::empty(),
    )
    .unwrap();

    let family_id = sock
        .resolve_genl_family("CONTROL_EXMPL")
        .expect("Generic Family must exist!");
    println!("Generic family number is {}", family_id);

    let echo_msg = "Some data that has `Nl` trait implemented, like &str";

    // We want to send an Echo command
    // 1) prepare Foobar Attribute
    let mut attrs: GenlBuffer<FoobarAttribute, Buffer> = GenlBuffer::new();
    attrs.push(
        Nlattr::new(
            // when is this not None? TODO
            None,
            false,
            // not sure, what this is TODO
            false,
            // the type of the attribute. This is an u16 and corresponds
            // to an enum on the receiving side
            FoobarAttribute::Msg,
            echo_msg,
        )
        .unwrap(),
    );
    // 2) prepare Generic Netlink Header. The Generic Netlink Header contains the
    //    attributes (actual data) as payload.
    let gnmsghdr = Genlmsghdr::new(
        // FoobarOperation::Echo,
        FoobarOperation::EchoFail,
        // Version is dependent on your kernel module
        // you can check it there and make special action, or you ignore it
        1,
        attrs,
    );
    // 3) Prepare Netlink header. The Netlink header contains the Generic Netlink header
    //    as payload.
    let nlmsghdr = Nlmsghdr::new(
        None,
        family_id,
        // This depends on your kernel module. Do you check there if any flags are used?
        // Request is required (TODO by neli or by netlink?)
        // others are up to you
        NlmFFlags::new(&[NlmF::Request]),
        None,
        // Port ID. Not necessarily the process id of the current process. This field
        // could be used to identify different points or threads inside your application
        // that send data to the kernel. This has nothing to do with "routing" the packet to
        // the kernel
        Some(process::id()),
        NlPayload::Payload(gnmsghdr),
    );

    println!("Sending '{}' via netlink", echo_msg);
    // Send data
    sock.send(nlmsghdr).expect("Send must work");

    // receive echo'ed message
    // TODO as of neli 0.5.1 .recv() is Err if nl_type of netlink header is 0x2 (NLMSG_ERR)
    //  I think this is wrong. A well formed error message should be Ok.
    let res: Nlmsghdr<u16, Genlmsghdr<FoobarOperation, FoobarAttribute>> =
        sock.recv().expect("Should receive a message").unwrap();

    // According to Netlink Linux kernel code nl_type is either 0x2 (error) or our family id (ok)
    if res.nl_type == 0x2 {
        // it actually depends on the kernel module if it replies with this nl_type
        // depends on your specific use case
        panic!("Kernel replied with error");
    }
    if res.nl_type == family_id {
        println!("Received successful reply!");
    }

    let attr_handle = res.get_payload().unwrap().get_attr_handle();
    let received = attr_handle.get_attribute(FoobarAttribute::Msg).unwrap();
    let received = String::deserialize(received.nla_payload.as_ref()).unwrap();
    println!("Received from kernel: '{}'", received);
}
