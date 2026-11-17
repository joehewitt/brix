#include <BxEvents/BxEvents.h>

#include <TargetConditionals.h>
#if TARGET_OS_OSX
#include <bsm/libbsm.h>
#endif
#include <mach/mach.h>
#if TARGET_OS_OSX
#include <servers/bootstrap.h>
#endif
#include <string.h>

static const int32_t bxMachParcelPortLimit = 8;
static const int32_t bxMachParcelByteLimit = 256;

struct BxMachParcelMessage {
    mach_msg_header_t header;
    mach_msg_body_t body;
    mach_msg_port_descriptor_t ports[bxMachParcelPortLimit];
    uint32_t size;
    uint8_t bytes[bxMachParcelByteLimit];
};

struct BxMachParcelReceipt {
    BxMachParcelMessage message;
    mach_msg_audit_trailer_t trailer;
};

extern "C" uint32_t
brixMachRendezvousCheckIn(const char* name) {
    if (!name) {
        return MACH_PORT_NULL;
    }
#if TARGET_OS_OSX
    mach_port_t port = MACH_PORT_NULL;
    if (bootstrap_check_in(bootstrap_port, name, &port) != KERN_SUCCESS) {
        return MACH_PORT_NULL;
    }
    mach_port_limits_t limits = {MACH_PORT_QLIMIT_LARGE};
    mach_port_set_attributes(
        mach_task_self(),
        port,
        MACH_PORT_LIMITS_INFO,
        reinterpret_cast<mach_port_info_t>(&limits),
        MACH_PORT_LIMITS_INFO_COUNT
    );
    return port;
#else
    return MACH_PORT_NULL;
#endif
}

extern "C" uint32_t
brixMachRendezvousLookUp(const char* name) {
    if (!name) {
        return MACH_PORT_NULL;
    }
#if TARGET_OS_OSX
    mach_port_t port = MACH_PORT_NULL;
    if (bootstrap_look_up(bootstrap_port, name, &port) != KERN_SUCCESS) {
        return MACH_PORT_NULL;
    }
    return port;
#else
    return MACH_PORT_NULL;
#endif
}

extern "C" void
brixMachRendezvousClose(uint32_t port) {
    if (port != MACH_PORT_NULL) {
        mach_port_mod_refs(mach_task_self(), port, MACH_PORT_RIGHT_RECEIVE, -1);
    }
}

extern "C" void
brixMachPortRelease(uint32_t port) {
    if (port != MACH_PORT_NULL) {
        mach_port_deallocate(mach_task_self(), port);
    }
}

extern "C" bool
brixMachSendParcel(uint32_t destination, const uint32_t* ports, int32_t portCount, const void* bytes, int64_t size) {
    if (destination == MACH_PORT_NULL || portCount < 0 || portCount > bxMachParcelPortLimit ||
            (portCount && !ports) || size < 0 || size > bxMachParcelByteLimit || (size && !bytes)) {
        for (int32_t index = 0; ports && index < portCount; ++index) {
            brixMachPortRelease(ports[index]);
        }
        return false;
    }
    BxMachParcelMessage message;
    memset(&message, 0, sizeof(message));
    message.header.msgh_bits = MACH_MSGH_BITS_SET(MACH_MSG_TYPE_COPY_SEND, 0, 0, MACH_MSGH_BITS_COMPLEX);
    message.header.msgh_size = sizeof(message);
    message.header.msgh_remote_port = destination;
    message.body.msgh_descriptor_count = static_cast<mach_msg_size_t>(bxMachParcelPortLimit);
    for (int32_t index = 0; index < bxMachParcelPortLimit; ++index) {
        message.ports[index].type = MACH_MSG_PORT_DESCRIPTOR;
        if (index < portCount) {
            message.ports[index].name = ports[index];
            message.ports[index].disposition = MACH_MSG_TYPE_MOVE_SEND;
        } else {
            message.ports[index].name = MACH_PORT_NULL;
            message.ports[index].disposition = MACH_MSG_TYPE_COPY_SEND;
        }
    }
    message.size = static_cast<uint32_t>(size);
    if (size) {
        memcpy(message.bytes, bytes, static_cast<size_t>(size));
    }
    kern_return_t result = mach_msg(
        &message.header,
        MACH_SEND_MSG | MACH_SEND_TIMEOUT,
        sizeof(message),
        0,
        MACH_PORT_NULL,
        0,
        MACH_PORT_NULL
    );
    if (result != MACH_MSG_SUCCESS) {
        if (result == MACH_SEND_TIMED_OUT || result == MACH_SEND_INTERRUPTED) {
            mach_msg_destroy(&message.header);
        } else {
            for (int32_t index = 0; index < portCount; ++index) {
                brixMachPortRelease(ports[index]);
            }
        }
        return false;
    }
    return true;
}

extern "C" bool
brixMachReceiveParcel(
    uint32_t port,
    uint32_t* ports,
    int32_t portCapacity,
    void* bytes,
    int64_t byteCapacity,
    int32_t* counts
) {
    if (port == MACH_PORT_NULL || !counts) {
        return false;
    }
    BxMachParcelReceipt receipt;
    memset(&receipt, 0, sizeof(receipt));
    kern_return_t result = mach_msg(
        &receipt.message.header,
        MACH_RCV_MSG | MACH_RCV_TIMEOUT | MACH_RCV_TRAILER_TYPE(MACH_MSG_TRAILER_FORMAT_0) |
            MACH_RCV_TRAILER_ELEMENTS(MACH_RCV_TRAILER_AUDIT),
        0,
        sizeof(receipt),
        port,
        0,
        MACH_PORT_NULL
    );
    if (result != MACH_MSG_SUCCESS) {
        return false;
    }
    int32_t received = 0;
    bool isParcel = (receipt.message.header.msgh_bits & MACH_MSGH_BITS_COMPLEX) &&
        receipt.message.header.msgh_size == sizeof(BxMachParcelMessage) &&
        receipt.message.body.msgh_descriptor_count == static_cast<mach_msg_size_t>(bxMachParcelPortLimit) &&
        receipt.message.size <= static_cast<uint32_t>(bxMachParcelByteLimit);
    if (!isParcel) {
        mach_msg_destroy(&receipt.message.header);
        return false;
    }
    for (int32_t index = 0; index < bxMachParcelPortLimit; ++index) {
        mach_port_t name = receipt.message.ports[index].name;
        if (name == MACH_PORT_NULL) {
            continue;
        }
        if (ports && received < portCapacity) {
            ports[received++] = name;
        } else {
            brixMachPortRelease(name);
        }
    }
    int64_t size = receipt.message.size;
    if (size > byteCapacity) {
        size = byteCapacity;
    }
    if (bytes && size > 0) {
        memcpy(bytes, receipt.message.bytes, static_cast<size_t>(size));
    }
    counts[0] = received;
    counts[1] = static_cast<int32_t>(size);
#if TARGET_OS_OSX
    counts[2] = audit_token_to_pid(receipt.trailer.msgh_audit);
#else
    counts[2] = static_cast<int32_t>(receipt.trailer.msgh_audit.val[5]);
#endif
    return true;
}
