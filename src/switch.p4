/* -*- P4_16 -*- */
#include <core.p4>
#include <v1model.p4>


/*****************************
 *  Constants
 ****************************/

// These have to be consistent with src/constants.py
#define MAX_HOPS 8
#define CPU_PORT 255

// Field list numbers
const bit<8> EMPTY_FL   = 0;
const bit<8> RESUB_FL   = 1;
const bit<8> CLONE_FL   = 2;
const bit<8> RECIRC_FL  = 3;

// EtherType (https://en.wikipedia.org/wiki/EtherType)
const bit<16>   TYPE_IPV4   = 0x0800;
const bit<16>   TYPE_ARP    = 0x0806;
const bit<16>   TYPE_VLAN   = 0x8100;
const bit<16>   TYPE_IPV6   = 0x86DD;

// IP protocol number (https://en.wikipedia.org/wiki/List_of_IP_protocol_numbers)
const bit<8>    PROTO_ICMP  = 1;
const bit<8>    PROTO_IPV4  = 4;
const bit<8>    PROTO_TCP   = 6;
const bit<8>    PROTO_UDP   = 17;
const bit<8>    PROTO_IPV6  = 41;
const bit<8>    PROTO_VERIFICATION = 0x91;


/*****************************
 *  Headers
 ****************************/

typedef bit<9>  egressSpec_t;
typedef bit<48> macAddr_t;
typedef bit<32> ip4Addr_t;

@controller_header("packet_in") // switch -> controller
header packet_in_t {
    bit<16>     invariantId;
}

@controller_header("packet_out") // controller -> switch
header packet_out_t {
    // bit<16>     egressPort;
}

header ethernet_t {
    macAddr_t   dstAddr;
    macAddr_t   srcAddr;
    bit<16>     etherType;
}

header ipv4_t {
    bit<4>      version;
    bit<4>      ihl;
    bit<8>      diffserv;
    bit<16>     totalLen;
    bit<16>     identification;
    bit<3>      flags;
    bit<13>     fragOffset;
    bit<8>      ttl;
    bit<8>      protocol;
    bit<16>     hdrChecksum;
    ip4Addr_t   srcAddr;
    ip4Addr_t   dstAddr;
}

header verification_t {
    bit<8>      protocol;       // the protocol in the original IPv4 header
    bit<16>     dfaState;       // state of the regex DFA
    // bit<8>      length;         // length (bytes) of the verification header, including traces
    // bit<8>      traceCount;     // number of traces // TODO: Why do we need both traceCount and length?
}

// header trace_t {
//     bit<16>     swId;
// }

header tcp_t {
    bit<16>     srcPort;
    bit<16>     dstPort;
    bit<32>     seqNo;
    bit<32>     ackNo;
    bit<4>      dataOffset;
    bit<3>      res;
    bit<3>      ecn;
    bit<6>      ctrl;
    bit<16>     window;
    bit<16>     checksum;
    bit<16>     urgentPtr;
}

header udp_t {
    bit<16>     srcPort;
    bit<16>     dstPort;
    bit<16>     length;
    bit<16>     checksum;
}

struct headers_t {
    packet_in_t         packet_in;
    packet_out_t        packet_out;
    ethernet_t          ethernet;
    ipv4_t              ipv4;
    verification_t      verification;
    // trace_t[MAX_HOPS]   traces; // TODO: Can it be of dynamic length?
    tcp_t               tcp;
    udp_t               udp;
}


/*****************************
 *  Metadata
 ****************************/

struct verification_metadata_t {
    bit<1>      entering;       // pkt just entered the network
    bit<1>      leaving;        // pkt is leaving the network (next hop is a host)
    // bit<8>      traceCounter;
    @field_list(RECIRC_FL)
    bit<1>      violating;      // pkt has violated an invariant
    @field_list(RECIRC_FL)
    bit<16>     invariantId;    // the violated invariant ID
}

struct metadata_t {
    bit<8>                  ecmp_group_id;
    bit<8>                  ecmp_select;
    verification_metadata_t verification;
}


/*****************************
 *  Parser
 ****************************/

parser MyParser(packet_in packet,
                out headers_t hdr,
                inout metadata_t meta,
                inout standard_metadata_t std_meta) {

    state start {
        // Initialize user-defined metadata that do not involve recirculation
        meta.ecmp_group_id = 0;
        meta.ecmp_select = 0;
        meta.verification.entering = 0;
        meta.verification.leaving = 0;
        // meta.verification.traceCounter = 0;

        transition select (std_meta.ingress_port) {
            CPU_PORT: parse_packet_out;
            default: parse_ethernet;
        }
    }

    state parse_packet_out {
        packet.extract(hdr.packet_out);
        transition parse_ethernet;
    }

    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select (hdr.ethernet.etherType) {
            TYPE_IPV4: parse_ipv4;
            default: accept;
        }
    }

    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition select (hdr.ipv4.protocol) {
            PROTO_TCP: parse_tcp;
            PROTO_UDP: parse_udp;
            PROTO_VERIFICATION: parse_verification;
            default: accept;
        }
    }

    state parse_verification {
        packet.extract(hdr.verification);
        transition select (hdr.verification.protocol) {
            PROTO_TCP: parse_tcp;
            PROTO_UDP: parse_udp;
            default: accept;
        }
        // meta.verification.traceCounter = hdr.verification.traceCount;
        // transition parse_trace_guard;
    }

    // state parse_trace_guard {
    //     transition select (meta.verification.traceCounter) {
    //         0: parse_L4;
    //         default: parse_trace;
    //     }
    // }

    // state parse_trace {
    //     packet.extract(hdr.traces.next);
    //     meta.verification.traceCounter = meta.verification.traceCounter - 1;
    //     transition parse_trace_guard;
    // }

    // state parse_L4 {
    //     transition select (hdr.verification.protocol) {
    //         PROTO_TCP: parse_tcp;
    //         PROTO_UDP: parse_udp;
    //         default: accept;
    //     }
    // }

    state parse_tcp {
        packet.extract(hdr.tcp);
        transition accept;
    }

    state parse_udp {
        packet.extract(hdr.udp);
        transition accept;
    }
}


/*****************************
 *  Checksum verification
 ****************************/

control MyVerifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {}
}


/*****************************
 *  Ingress processing
 ****************************/

control ipv4_forwarding(
    inout headers_t hdr,
    inout metadata_t meta,
    inout standard_metadata_t std_meta
) {
    action ipv4_forward(macAddr_t dstAddr, egressSpec_t port) {
        std_meta.egress_spec = port;
        hdr.ethernet.srcAddr = hdr.ethernet.dstAddr;
        hdr.ethernet.dstAddr = dstAddr;
        hdr.ipv4.ttl = hdr.ipv4.ttl - 1;
    }

    action ecmp_select(bit<8> ecmp_group_id, bit<8> ecmp_count) {
        bit<8> protocol = hdr.ipv4.protocol;
        bit<16> srcPort = 0;
        bit<16> dstPort = 0;

        if (hdr.tcp.isValid()) {
            protocol = PROTO_TCP;
            srcPort = hdr.tcp.srcPort;
            dstPort = hdr.tcp.dstPort;
        } else if (hdr.udp.isValid()) {
            protocol = PROTO_UDP;
            srcPort = hdr.udp.srcPort;
            dstPort = hdr.udp.dstPort;
        }

        hash(
            meta.ecmp_select,
            HashAlgorithm.crc16,
            1w0,
            {
                hdr.ipv4.srcAddr,
                hdr.ipv4.dstAddr,
                protocol,
                srcPort,
                dstPort
            },
            ecmp_count
        );
        meta.ecmp_group_id = ecmp_group_id;
    }

    action drop() {
        mark_to_drop(std_meta);
    }

    table ipv4_lpm {
        key = {
            hdr.ipv4.dstAddr: lpm;
        }
        actions = {
            ipv4_forward;
            ecmp_select;
            drop;
            NoAction;
        }
        size = 1024;
        default_action = drop();
    }

    table ecmp_forward {
        key = {
            meta.ecmp_group_id: exact;
            meta.ecmp_select: exact;
        }
        actions = {
            ipv4_forward;
        }
        size = 512;
    }

    apply {
        if (hdr.ipv4.isValid()) {
            switch (ipv4_lpm.apply().action_run) {
                ecmp_select: {
                    ecmp_forward.apply();
                }
            }

            if (hdr.ipv4.ttl == 0) {
                drop();
            }
        }
    }
}

control MyIngress(
    inout headers_t hdr,
    inout metadata_t meta,
    inout standard_metadata_t std_meta
) {
    action insert_verification_header() {
        hdr.verification.setValid();
        hdr.verification.protocol = hdr.ipv4.protocol;
        hdr.verification.dfaState = 0;
        // hdr.verification.length = 5;
        // hdr.verification.traceCount = 0;
        hdr.ipv4.totalLen = hdr.ipv4.totalLen + 3;
        hdr.ipv4.protocol = PROTO_VERIFICATION;
        meta.verification.entering = 1;
    }

    table encapsulation {
        key = {
            hdr.ipv4.protocol: range;
        }
        actions = {
            insert_verification_header;
            NoAction;
        }
        size = 2;
        default_action = NoAction();
    }

    action violate(bit<16> invId) {
        std_meta.egress_spec = CPU_PORT;
        meta.verification.violating = 1;
        meta.verification.invariantId = invId;
        hdr.packet_in.setValid();
        hdr.packet_in.invariantId = invId;
    }

    // table loop {
    //     key = {
    //         hdr.verification.traceCount: range;
    //         hdr.traces[0].swId: range;
    //         hdr.traces[1].swId: range;
    //         hdr.traces[2].swId: range;
    //         hdr.traces[3].swId: range;
    //         hdr.traces[4].swId: range;
    //         hdr.traces[5].swId: range;
    //         hdr.traces[6].swId: range;
    //         hdr.traces[7].swId: range;
    //     }
    //     actions = {
    //         violate;
    //         NoAction;
    //     }
    //     size = 64;
    //     default_action = NoAction();
    // }

    action initial_transition(bit<16> state) {
        hdr.verification.dfaState = state;
    }

    table regex_init {
        key = {
            hdr.ipv4.srcAddr: range;
            hdr.ipv4.dstAddr: range;
            meta.verification.entering: exact;
            std_meta.ingress_port: range;
        }
        actions = {
            initial_transition;
            violate;
            NoAction;
        }
        size = 512;
        default_action = NoAction();
    }

    action regex_trans(bit<16> state) {
        hdr.verification.dfaState = state;
    }

    table regex_transition {
        key = {
            hdr.ipv4.srcAddr: range;
            hdr.ipv4.dstAddr: range;
            hdr.verification.dfaState: range;
            std_meta.egress_spec: exact;
        }
        actions = {
            regex_trans;
            NoAction;
        }
        size = 512;
        default_action = NoAction();
    }

    // action add_trace(bit<16> add, bit<16> swId) {
    //     hdr.ipv4.totalLen = hdr.ipv4.totalLen + (2 * add);
    //     hdr.verification.length = hdr.verification.length + (bit<8>)(2 * add);
    //     hdr.verification.traceCount = hdr.verification.traceCount + (bit<8>)add;
    //     hdr.traces.push_front(1);
    //     hdr.traces[0].setValid();
    //     hdr.traces[0].swId = swId;
    // }

    // table trace {
    //     key = {
    //         hdr.verification.traceCount: range;
    //         meta.verification.leaving: exact;
    //     }
    //     actions = {
    //         add_trace;
    //         NoAction;
    //     }
    //     size = 4;
    //     default_action = NoAction();
    // }

    apply {
        // Handle recirculated packets
        if (meta.verification.violating == 1) {
            violate(meta.verification.invariantId);
            return;
        }

        // Pre-ingress check
        encapsulation.apply();
        if (meta.verification.violating == 1) {
            return;
        }
        // loop.apply();
        // if (meta.verification.violating == 1) {
        //     return;
        // }
        regex_init.apply();
        if (meta.verification.violating == 1) {
            return;
        }
        // trace.apply();

        // Original packet processing of the switch
        ipv4_forwarding.apply(hdr, meta, std_meta);

        // Post-ingress check
        regex_transition.apply();
        if (meta.verification.violating == 1) {
            return;
        }
    }
}


/*****************************
 *  Egress processing
 ****************************/

control MyEgress(
    inout headers_t hdr,
    inout metadata_t meta,
    inout standard_metadata_t std_meta
) {
    action mark_leaving(){
        meta.verification.leaving = 1;
    }

    table check_leaving {
        key = {
            std_meta.egress_port: exact;
        }
        actions = {
            mark_leaving;
            NoAction;
        }
        size = 64;
        default_action = NoAction();
    }

    action remove_verification_header() {
        hdr.ipv4.protocol = hdr.verification.protocol;
        hdr.ipv4.totalLen = hdr.ipv4.totalLen - 3;
        hdr.verification.setInvalid();
        // hdr.traces[0].setInvalid();
        // hdr.traces[1].setInvalid();
        // hdr.traces[2].setInvalid();
        // hdr.traces[3].setInvalid();
        // hdr.traces[4].setInvalid();
        // hdr.traces[5].setInvalid();
        // hdr.traces[6].setInvalid();
        // hdr.traces[7].setInvalid();
    }

    table decapsulation {
        key = {
            meta.verification.leaving: exact;
        }
        actions = {
            remove_verification_header;
            NoAction;
        }
        size = 2;
        default_action = NoAction();
    }

    // For details about reporting packets in egress:
    // https://github.com/kyechou/allie/issues/6
    action violate(bit<16> invId) {
        meta.verification.violating = 1;
        meta.verification.invariantId = invId;
        recirculate_preserving_field_list(RECIRC_FL);
    }

    table segmentation {
        key = {
            hdr.ipv4.srcAddr: range;
            hdr.ipv4.dstAddr: range;
            std_meta.egress_spec: range;
            std_meta.egress_port: range;
        }
        actions = {
            violate;
            NoAction;
        }
        size = 512;
        default_action = NoAction();
    }

    action regex_trans(bit<16> state) {
        hdr.verification.dfaState = state;
    }

    table regex_transition {
        key = {
            hdr.ipv4.srcAddr: range;
            hdr.ipv4.dstAddr: range;
            hdr.verification.dfaState: range;
            std_meta.egress_spec: range;
            std_meta.egress_port: range;
        }
        actions = {
            regex_trans;
            violate;
            NoAction;
        }
        size = 512;
        default_action = NoAction();
    }

    table regex_terminate {
        key = {
            hdr.ipv4.srcAddr: range;
            hdr.ipv4.dstAddr: range;
            meta.verification.leaving: exact;
            hdr.verification.dfaState: range;
        }
        actions = {
            violate;
            NoAction;
        }
        size = 512;
        default_action = NoAction();
    }

    apply {
        // Original packet processing of the switch
        // (Nothing in this example)

        // Post-egress check
        check_leaving.apply();  // mark if the packet is leaving the network
        segmentation.apply();   // check for segmentation
        if (meta.verification.violating == 1) {
            return;
        }
        regex_transition.apply();
        if (meta.verification.violating == 1) {
            return;
        }
        regex_terminate.apply();
        if (meta.verification.violating == 1) {
            return;
        }
        decapsulation.apply();
    }
}


/*****************************
 *  Checksum computation
 ****************************/

control MyComputeChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
        update_checksum(
            hdr.ipv4.isValid(),
            {
                hdr.ipv4.version,
                hdr.ipv4.ihl,
                hdr.ipv4.diffserv,
                hdr.ipv4.totalLen,
                hdr.ipv4.identification,
                hdr.ipv4.flags,
                hdr.ipv4.fragOffset,
                hdr.ipv4.ttl,
                hdr.ipv4.protocol,
                hdr.ipv4.srcAddr,
                hdr.ipv4.dstAddr
            },
            hdr.ipv4.hdrChecksum,
            HashAlgorithm.csum16
        );
    }
}


/*****************************
 *  Deparser
 ****************************/

control MyDeparser(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit(hdr.packet_in);
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.verification);
        // packet.emit(hdr.traces);
        packet.emit(hdr.tcp);
        packet.emit(hdr.udp);
    }
}


/*****************************
 *  Switch
 ****************************/

V1Switch(
    MyParser(),
    MyVerifyChecksum(),
    MyIngress(),
    MyEgress(),
    MyComputeChecksum(),
    MyDeparser()
) main;
