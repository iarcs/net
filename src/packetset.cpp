#include "packetset.hpp"

#include "ipv4.hpp"
#include "util.hpp"

using namespace std;

PacketSet::PacketSet() {
    ;
}

PacketSet::PacketSet(sj_object root __attribute__((unused))) {
    // TODO
}

vector<string> PacketSet::srcIPRange() const {
    // TODO
    // (placeholder)
    IPAddress low("0.0.0.0"), high("255.255.255.254");
    return {uint_to_be_str(low.value()), uint_to_be_str(high.value())};
}

vector<string> PacketSet::dstIPRange() const {
    // TODO
    // (placeholder)
    IPAddress low("0.0.0.0"), high("255.255.255.254");
    return {uint_to_be_str(low.value()), uint_to_be_str(high.value())};
}
