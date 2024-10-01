#pragma once

#include <string>

#include "invariant.hpp"

class SegmentationInvariant : public Invariant {
private:
    std::string _switch;
    uint16_t _port;

public:
    SegmentationInvariant() = default;
    SegmentationInvariant(uint16_t id, sj_object root);

    std::unique_ptr<Invariant> clone(uint16_t id,
                                     sj_object root) const override;
    std::unordered_map<std::shared_ptr<Switch>,
                       std::unordered_set<P4TableEntry>>
    getTableEntries(const Network &) const override;
};
