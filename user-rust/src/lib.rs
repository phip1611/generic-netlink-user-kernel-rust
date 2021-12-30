use neli::neli_enum;

/// Name of the Netlink family registered via Generic Netlink
pub const FAMILY_NAME: &str = "gnl_foobar_xmpl";

/// This is for the "cmd" field in Generic Netlink header.
/// NlFoobarXmplCommand corresponds to "enum GNlFoobarXmplCommand" in kernel module C code.
/// Describes what callback function shall be invoked in the linux kernel module.
///
/// For full documentation of the fields, please head to "gnl_foobar_xmpl_prop.h",
/// enum `GNL_FOOBAR_XMPL_COMMAND`.
#[neli_enum(serialized_type = "u8")]
pub enum NlFoobarXmplCommand {
    Unspec = 0,
    // When this command is received, we expect the attribute `GNlFoobarXmplAttribute::GNL_FOOBAR_XMPL_A_MSG` to
    // be present in the Generic Netlink message.
    EchoMsg = 1,
    // Provokes a NLMSG_ERR answer to this request as described in netlink manpage
    // (https://man7.org/linux/man-pages/man7/netlink.7.html).
    ReplyWithNlmsgErr = 2,
}
impl neli::consts::genl::Cmd for NlFoobarXmplCommand {}

/// Implements the necessary trait for the "neli" lib on an enum called "NlFoobarXmplAttribute".
/// NlFoobarXmplAttribute corresponds to "enum NlFoobarXmplAttribute" in kernel module C code.
/// Describes the value type to data mappings inside the generic netlink packet payload.
/// This is for the Netlink Attributes (the actual payload) we want to send.
///
/// For full documentation of the fields, please head to "gnl_foobar_xmpl_prop.h",
/// enum `GNL_FOOBAR_XMPL_ATTRIBUTE`.
#[neli_enum(serialized_type = "u16")]
pub enum NlFoobarXmplAttribute {
    Unspec = 0,
    // We expect a MSG to be a null-terminated C-string.
    Msg = 1,
}
impl neli::consts::genl::NlAttrType for NlFoobarXmplAttribute {}
