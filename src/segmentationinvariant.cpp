#include "segmentationinvariant.hpp"

#include "constants.hpp"
#include "util.hpp"

using namespace std;

SegmentationInvariant::SegmentationInvariant(uint16_t id, sj_object root)
    : Invariant(id, root) {
    this->_switch = string_view(root["switch"].get_string());
    this->_port = root["port"].get_uint64();
}

unique_ptr<Invariant> SegmentationInvariant::clone(uint16_t id,
                                                   sj_object root) const {
    return make_unique<SegmentationInvariant>(id, root);
}

unordered_map<shared_ptr<Switch>, unordered_set<P4TableEntry>>
SegmentationInvariant::getTableEntries(const Network &network) const {
    auto it = network.switches().find(this->_switch);

    if (it == network.switches().end()) {
        logger.warn("Switch not found: " + this->_switch);
        return {};
    }

    auto sw = it->second;
    unordered_set<P4TableEntry> entries;

    {
        P4TableEntry entry;
        entry.tableName("MyEgress.segmentation");
        entry.addMatchField("hdr.ipv4.srcAddr", this->_ps.srcIPRange());
        entry.addMatchField("hdr.ipv4.dstAddr", this->_ps.dstIPRange());
        entry.addMatchField(
            "std_meta.egress_spec",
            {uint_to_be_str(DROP_PORT), uint_to_be_str(DROP_PORT)});
        entry.actionName("NoAction");
        entry.priority(Priority::High);
        entries.emplace(std::move(entry));
    }

    {
        P4TableEntry entry;
        entry.tableName("MyEgress.segmentation");
        entry.addMatchField("hdr.ipv4.srcAddr", this->_ps.srcIPRange());
        entry.addMatchField("hdr.ipv4.dstAddr", this->_ps.dstIPRange());
        entry.addMatchField(
            "std_meta.egress_spec",
            {uint_to_be_str(this->_port), uint_to_be_str(this->_port)});
        entry.actionName("MyEgress.violate");
        entry.addActionParam("invId", uint_to_be_str(this->_id));
        entry.priority(Priority::Low);
        entries.emplace(std::move(entry));
    }

    unordered_map<shared_ptr<Switch>, unordered_set<P4TableEntry>> rules;
    rules.emplace(sw, std::move(entries));
    return rules;
}
