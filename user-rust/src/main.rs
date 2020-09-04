//! Make sure to make familiar with netlink and generic netlink first!
//! I personally also just know a few things! If something is wrong please let me know!

//! This code is the user part written in Rust that talks via a generic netlink family called
//! "CONTROL_EXMPL" to a linux kernel module. It uses the "neli" lib/crate.
//! 
//! Each netlink packet has a generic netlink packet as it's payload. In the generic netlink
//! package one can define several attributes (type to value mappings).
//! 
//! Workflow is the following:
//! 0) kernel module registers generic netlink family by name which creates a new 
//!    virtual/temporary netlink family (a number)
//! 1) client request family id of "CONTROL_EXMPL"
//! 2) client sends data to retrieved family id.

#[macro_use]
extern crate neli;

use neli::socket;
use neli::consts::{NlFamily, Cmd, NlAttrType, NlmF};
use neli::genl::Genlmsghdr;
use neli::nlattr::Nlattr;
use neli::nl::Nlmsghdr;
use std::process;

// Implements the necessary trait for the "neli" lib on an enum called "Command".
// Command corresponds to "enum Commands" in kernel module C code.
// Describes what callback function shall be invoked in the linux kernel module.
impl_var_trait!(
    Command, u8, Cmd,
    Unspec => 0,
    Echo => 1
);

// Implements the necessary trait for the "neli" lib on an enum called "ControlAttr".
// Command corresponds to "enum Attributes" in kernel module C code.
// Describes the value type to data mappings inside the generic netlink packet payload.
impl_var_trait!(
    ControlAttr, u16, NlAttrType,
    Unspec => 0,
    Msg => 1
);

fn main() {
    let mut socket = socket::NlSocket::connect(NlFamily::Generic, None, None, true).unwrap();
    let family_id = socket.resolve_genl_family("CONTROL_EXMPL").expect("Generic Family must exist!");
    println!("Generic family number is {}", family_id);

    // Send String to linux kernel module
    let str = String::from("Hello from userland (Rust)");
    let attr = Nlattr::new(
        Some(str.len() as u16),
        ControlAttr::Msg,
        str.clone()
    ).unwrap();
    let attrs = vec![attr];
    let gen_nl_mhr = Genlmsghdr::new(Command::Echo, 1, attrs).unwrap();
    let nl_msh_flags = vec![NlmF::Request];
    let nl_msh = Nlmsghdr::new(
        None,
        family_id,
        nl_msh_flags,
        None,
        Some(process::id()),
        gen_nl_mhr
    );
    println!("Sending '{}' via netlink", str);
    socket.send_nl(nl_msh).unwrap();

    // ----------------------------------------
    // Receive data;
    // u16 is the family id; theoretically we could assert that
    // res.nl_type == family_id
    let res = socket.recv_nl::<u16, Genlmsghdr::<Command, ControlAttr>>(None).unwrap();
    // iterate through all received attributes
    res.nl_payload.get_attr_handle().iter().for_each(|attr| {
        match attr.nla_type {
           ControlAttr::Msg => {
                println!("{}", String::from_utf8(attr.payload.clone()).unwrap());
           },
            _ => panic!()
       }
    });
}

