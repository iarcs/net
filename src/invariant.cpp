#include "invariant.hpp"

#include "logger.hpp"
#include "loopinvariant.hpp"
#include "regexinvariant.hpp"
#include "segmentationinvariant.hpp"

using namespace std;

Invariant::Invariant(uint16_t id, sj_object root) : _id(id) {
    this->_name = string_view(root["name"].get_string());
    this->_type = string_view(root["type"].get_string());
    this->_ps = PacketSet(root["packet_set"].get_object());
}

InvariantsParser::InvariantsParser() {
    this->_baseInvs.emplace("loop", make_unique<LoopInvariant>());
    this->_baseInvs.emplace("regex", make_unique<RegexInvariant>());
    this->_baseInvs.emplace("segmentation",
                            make_unique<SegmentationInvariant>());
}

vector<unique_ptr<Invariant>>
InvariantsParser::operator()(const string &invSpec) const {
    logger.info("Loading invariants specification");

    simdjson::ondemand::parser parser;
    simdjson::padded_string json = simdjson::padded_string::load(invSpec);
    simdjson::ondemand::document root = parser.iterate(json);

    vector<unique_ptr<Invariant>> invs;
    uint16_t invId = 0;

    for (auto field : root.get_array()) {
        auto invJson = field.get_object();
        auto invType = string(string_view(invJson["type"].get_string()));
        auto it = this->_baseInvs.find(invType);

        if (it == this->_baseInvs.end()) {
            logger.error("Invalid invariant type " + invType);
        }

        invs.emplace_back(it->second->clone(invId, invJson));
        ++invId;
    }

    return invs;
}
