#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <simdjson.h>

#include "hash.hpp"
#include "interface.hpp"
#include "node.hpp"

using sj_object = simdjson::simdjson_result<simdjson::ondemand::object>;

class Network;

class Link {
private:
    const Network *_network; // parent network
    std::shared_ptr<Node> _node1, _node2;
    std::shared_ptr<Interface> _intf1, _intf2;

    friend bool operator==(const Link &, const Link &) noexcept = default;

public:
    Link(sj_object root, Network *network);

    decltype(_network) network() const { return _network; }
    const decltype(_node1) &node1() const { return _node1; }
    const decltype(_node2) &node2() const { return _node2; }
    const decltype(_intf1) &intf1() const { return _intf1; }
    const decltype(_intf2) &intf2() const { return _intf2; }
};

namespace std {

template <>
struct hash<Link> {
    size_t operator()(const Link &link) const {
        size_t value = 0;
        hash<shared_ptr<Node>> nodePtrHasher;
        hash<shared_ptr<Interface>> intfPtrHasher;
        value = ::hash::hash_combine(nodePtrHasher(link.node1()), value);
        value = ::hash::hash_combine(nodePtrHasher(link.node2()), value);
        value = ::hash::hash_combine(intfPtrHasher(link.intf1()), value);
        value = ::hash::hash_combine(intfPtrHasher(link.intf2()), value);
        return value;
    }
};

} // namespace std
