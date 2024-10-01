#include "link.hpp"

#include <utility>

#include "network.hpp"

using namespace std;

Link::Link(sj_object root, Network *network) : _network(network) {
    assert(!root["node1"].is_null());
    assert(!root["node2"].is_null());
    assert(!root["port1"].is_null());
    assert(!root["port2"].is_null());
    assert(!root["intf1"].is_null());
    assert(!root["intf2"].is_null());

    const auto &nodes = this->_network->nodes();
    this->_node1 = nodes.at(string(string_view(root["node1"].get_string())));
    this->_node2 = nodes.at(string(string_view(root["node2"].get_string())));
    this->_intf1 = this->_node1->getIntfByPort(root["port1"].get_uint64());
    this->_intf2 = this->_node2->getIntfByPort(root["port2"].get_uint64());
    assert(this->_intf1->name() == string_view(root["intf1"].get_string()));
    assert(this->_intf2->name() == string_view(root["intf2"].get_string()));

    // Normalize the ordering
    if (this->_node2->name() < this->_node1->name() ||
        (this->_node1->name() == this->_node2->name() &&
         this->_intf2->port() < this->_intf1->port())) {
        swap(this->_node1, this->_node2);
        swap(this->_intf1, this->_intf2);
    }

    // Populate Interface::_neighborNode, Interface::_neighborIntf, and
    // Node::_intfsByNeighborNode
    this->_intf1->neighborNode(this->_node2);
    this->_intf1->neighborIntf(this->_intf2);
    this->_intf2->neighborNode(this->_node1);
    this->_intf2->neighborIntf(this->_intf1);
    this->_node1->addIntfNeighborRel(this->_node2->name(), this->_intf1);
    this->_node2->addIntfNeighborRel(this->_node1->name(), this->_intf2);
}
