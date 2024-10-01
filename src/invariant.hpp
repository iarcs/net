#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <simdjson.h>

#include "network.hpp"
#include "packetset.hpp"

using sj_object = simdjson::simdjson_result<simdjson::ondemand::object>;

class Invariant {
protected:
    uint16_t _id;
    std::string _name;
    std::string _type;
    PacketSet _ps;

public:
    Invariant() = default;
    Invariant(uint16_t id, sj_object root);
    Invariant(const Invariant &) = delete;
    Invariant(Invariant &&) = default;
    virtual ~Invariant() = default;

    decltype(_id) id() const { return _id; }
    const decltype(_name) name() const { return _name; }
    const decltype(_type) type() const { return _type; }
    const decltype(_ps) packetSet() const { return _ps; }

    virtual std::unique_ptr<Invariant> clone(uint16_t id,
                                             sj_object root) const = 0;
    virtual std::unordered_map<std::shared_ptr<Switch>,
                               std::unordered_set<P4TableEntry>>
    getTableEntries(const Network &) const = 0;
};

class InvariantsParser {
private:
    std::unordered_map<std::string, std::unique_ptr<Invariant>> _baseInvs;

public:
    InvariantsParser();

    std::vector<std::unique_ptr<Invariant>>
    operator()(const std::string &invSpec) const;
};
