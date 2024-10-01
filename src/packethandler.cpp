#include "packethandler.hpp"

#include "controller.hpp"
#include "logger.hpp"
#include "p4runtimemgr.hpp"
#include "switch.hpp"
#include "util.hpp"

using namespace std;

PacketHandler::PacketHandler(const Controller *controller,
                             const Switch *sw,
                             const P4RuntimeMgr *p4rtmanager,
                             std::string &&invId,
                             std::string &&frame)
    : _controller(controller), _switch(sw), _p4rtmanager(p4rtmanager),
      _invId(be_str_to_u16(invId)), _frame(frame) {}

void PacketHandler::operator()() {
    // this->_switch->swLogger()->info("======================================");

    logger.warn("Violation occurred at " + this->_switch->name() +
                ", Invariant " + to_string(this->_invId));
    this->_switch->swLogger()->warn("Violation occurred at " +
                                    this->_switch->name() + ", Invariant " +
                                    to_string(this->_invId));

    // TODO
    // pkt_bytes = packet.payload
    // eth = ethernet.Ethernet(pkt_bytes)
    // print(eth)

    // print('Violated invariant:')
    // print(self.invariants[invId])

    /**
     *  simple_router_mgr.cpp
     */

    // char *pkt = pkt_copy.data();
    // size_t size = pkt_copy.size();
    // size_t offset = 0;
    // cpu_header_t cpu_hdr;
    // if ((size - offset) < sizeof(cpu_hdr))
    //     return;
    // char zeros[8];
    // memset(zeros, 0, sizeof(zeros));
    // if (memcmp(zeros, pkt, sizeof(zeros)))
    //     return;
    // memcpy(&cpu_hdr, pkt, sizeof(cpu_hdr));
    // cpu_hdr.reason = ntohs(cpu_hdr.reason);
    // cpu_hdr.port = ntohs(cpu_hdr.port);
    // offset += sizeof(cpu_hdr);
    // if ((size - offset) < sizeof(eth_header_t))
    //     return;
    // offset += sizeof(eth_header_t);
    // if (cpu_hdr.reason == NO_ARP_ENTRY) {
    //     if ((size - offset) < sizeof(ipv4_header_t))
    //         return;
    //     ipv4_header_t ip_hdr;
    //     memcpy(&ip_hdr, pkt + offset, sizeof(ip_hdr));
    //     ip_hdr.dst_addr = ntohl(ip_hdr.dst_addr);
    //     simple_router_mgr->handle_ip(std::move(pkt_copy),
    //     ip_hdr.dst_addr);
    // } else if (cpu_hdr.reason == ARP_MSG) {
    //     if ((size - offset) < sizeof(arp_header_t))
    //         return;
    //     arp_header_t arp_header;
    //     memcpy(&arp_header, pkt + offset, sizeof(arp_header));
    //     arp_header.hw_type = ntohs(arp_header.hw_type);
    //     arp_header.proto_type = ntohs(arp_header.proto_type);
    //     arp_header.opcode = ntohs(arp_header.opcode);
    //     arp_header.proto_src_addr = ntohl(arp_header.proto_src_addr);
    //     arp_header.proto_dst_addr = ntohl(arp_header.proto_dst_addr);
    //     simple_router_mgr->handle_arp(arp_header);
    // }
}
