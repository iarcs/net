#pragma once

#include <memory>
#include <set>
#include <string>
#include <sys/types.h>
#include <unordered_map>
#include <unordered_set>

#include <simdjson.h>

#include "interface.hpp"
#include "ipv4.hpp"
#include "rule.hpp"

using sj_object = simdjson::simdjson_result<simdjson::ondemand::object>;

class Network;

class Node {
protected:
    const Network *_network; // parent network
    std::string _name;
    pid_t _pid;
    std::unordered_map<std::string, std::shared_ptr<Interface>> _intfsByName;
    std::unordered_map<int, std::shared_ptr<Interface>> _intfsByPort;
    std::unordered_map<std::string,
                       std::unordered_set<std::shared_ptr<Interface>>>
        _intfsByNeighborNode;

public:
    Node(sj_object root, Network *network);
    virtual ~Node() = default;

    decltype(_network) network() const { return _network; }
    decltype(_name) name() const { return _name; }
    decltype(_pid) pid() const { return _pid; }
    const std::shared_ptr<Interface> &getIntfByName(const std::string &) const;
    const std::shared_ptr<Interface> &getIntfByPort(int) const;
    const std::unordered_set<std::shared_ptr<Interface>> *
    getIntfsByNeighborNode(const std::string &) const;

    void addIntfNeighborRel(const std::string &,
                            const std::shared_ptr<Interface> &);

    virtual size_t
    installRules(const std::unordered_map<IPNetwork, std::set<Rule>> &) = 0;
};
