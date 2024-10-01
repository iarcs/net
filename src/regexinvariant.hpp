#pragma once

#include <string>

#include "invariant.hpp"

class RegexInvariant : public Invariant {
private:
    std::string _pattern;

public:
    RegexInvariant() = default;
    RegexInvariant(uint16_t id, sj_object root);

    std::unique_ptr<Invariant> clone(uint16_t id,
                                     sj_object root) const override;
    std::unordered_map<std::shared_ptr<Switch>,
                       std::unordered_set<P4TableEntry>>
    getTableEntries(const Network &) const override;
};
