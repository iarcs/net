#pragma once

#include <simdjson.h>

#include "node.hpp"

using sj_object = simdjson::simdjson_result<simdjson::ondemand::object>;

class Network;

class Host : public Node {
private:
    int _hostId;

public:
    Host(sj_object root, Network *network);
    ~Host() override = default;

    size_t installRules(
        const std::unordered_map<IPNetwork, std::set<Rule>> &) override;
};
