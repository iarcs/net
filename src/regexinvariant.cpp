#include "regexinvariant.hpp"

using namespace std;

RegexInvariant::RegexInvariant(uint16_t id, sj_object root)
    : Invariant(id, root) {
    this->_pattern = string_view(root["pattern"].get_string());
}

unique_ptr<Invariant> RegexInvariant::clone(uint16_t id, sj_object root) const {
    return make_unique<RegexInvariant>(id, root);
}

unordered_map<shared_ptr<Switch>, unordered_set<P4TableEntry>>
RegexInvariant::getTableEntries(const Network &network
                                __attribute__((unused))) const {
    // TODO
    return {};
}
