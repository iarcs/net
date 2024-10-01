#include "loopinvariant.hpp"

using namespace std;

LoopInvariant::LoopInvariant(uint16_t id, sj_object root)
    : Invariant(id, root) {}

unique_ptr<Invariant> LoopInvariant::clone(uint16_t id, sj_object root) const {
    return make_unique<LoopInvariant>(id, root);
}

unordered_map<shared_ptr<Switch>, unordered_set<P4TableEntry>>
LoopInvariant::getTableEntries(const Network &network
                               __attribute__((unused))) const {
    // TODO
    return {};
}
