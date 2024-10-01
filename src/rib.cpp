#include "rib.hpp"

using namespace std;

RIB::RIB(sj_object root) {
    // TODO

    for (auto field : root) {
        auto name = string_view(field.unescaped_key());

        auto it = this->_tbl.find(string(name));
        if (it == this->_tbl.end()) {
            auto res =
                this->_tbl.emplace(name, unordered_map<IPNetwork, set<Rule>>());
            assert(res.second);
            it = res.first;
        }

        unordered_map<IPNetwork, set<Rule>> &prefixMap = it->second;

        for (auto ruleJson : field.value().get_array()) {
            Rule rule(ruleJson.get_object());
            auto it = prefixMap.find(rule.prefix());
            if (it == prefixMap.end()) {
                auto res = prefixMap.emplace(rule.prefix(), set<Rule>());
                assert(res.second);
                it = res.first;
            }
            it->second.emplace(std::move(rule));
        }
    }
}
