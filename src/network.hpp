#pragma once

#include <memory>
#include <set>
#include <string>
#include <unordered_map>

#include "host.hpp"
#include "link.hpp"
#include "node.hpp"
#include "rib.hpp"
#include "switch.hpp"

class Controller;

class Network {
private:
    const Controller *_controller; // parent controller
    std::unordered_map<std::string, std::shared_ptr<Node>> _nodes;
    std::unordered_map<std::string, std::shared_ptr<Host>> _hosts;
    std::unordered_map<std::string, std::shared_ptr<Switch>> _switches;
    std::unordered_map<std::string, std::unordered_set<std::shared_ptr<Node>>>
        _nodeGroups;
    std::unordered_set<Link> _links;
    RIB _rib;

public:
    Network(Controller *controller);

    void init(const std::string &networkSpec);

    decltype(_controller) controller() const { return _controller; }
    const decltype(_nodes) &nodes() const { return _nodes; }
    const decltype(_hosts) &hosts() const { return _hosts; }
    const decltype(_switches) &switches() const { return _switches; }
    const decltype(_nodeGroups) &nodeGroups() const { return _nodeGroups; }
    const decltype(_links) &links() const { return _links; }
    const decltype(_rib) &rib() const { return _rib; }
};
