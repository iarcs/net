#include "node.hpp"

#include "logger.hpp"
#include "network.hpp"

using namespace std;

Node::Node(sj_object root, Network *network) : _network(network) {
    this->_name = string_view(root["name"].get_string());
    this->_pid = root["pid"].get_uint64();

    simdjson::ondemand::object obj;

    if (!root["intfs"].get(obj)) {
        for (auto field : root["intfs"].get_object()) {
            auto name = string_view(field.unescaped_key());
            auto intf =
                make_shared<Interface>(field.value().get_object(), this);

            if (!this->_intfsByName.emplace(name, intf).second) {
                logger.error("Duplicate interface " + string(name));
            }

            if (!this->_intfsByPort.emplace(intf->port(), intf).second) {
                logger.error("Duplicate interface port " +
                             to_string(intf->port()));
            }
        }
    }
}

const shared_ptr<Interface> &Node::getIntfByName(const string &name) const {
    auto it = this->_intfsByName.find(name);
    if (it == this->_intfsByName.end()) {
        logger.error("Interface not found: " + name);
    }
    return it->second;
}

const shared_ptr<Interface> &Node::getIntfByPort(int port) const {
    auto it = this->_intfsByPort.find(port);
    if (it == this->_intfsByPort.end()) {
        logger.error("Interface not found: " + to_string(port));
    }
    return it->second;
}

const unordered_set<shared_ptr<Interface>> *
Node::getIntfsByNeighborNode(const string &neighborNode) const {
    auto it = this->_intfsByNeighborNode.find(neighborNode);
    if (it == this->_intfsByNeighborNode.end()) {
        return nullptr;
    } else {
        return &(it->second);
    }
}

void Node::addIntfNeighborRel(const string &neighborNode,
                              const shared_ptr<Interface> &intf) {
    assert(this->_intfsByPort.count(intf->port()) > 0);

    auto it = this->_intfsByNeighborNode.find(neighborNode);
    if (it == this->_intfsByNeighborNode.end()) {
        unordered_set<shared_ptr<Interface>> intfs;
        intfs.insert(intf);
        this->_intfsByNeighborNode.emplace(neighborNode, std::move(intfs));
    } else {
        assert(it->second.insert(intf).second);
    }
}
