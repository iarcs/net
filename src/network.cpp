#include "network.hpp"

#include <simdjson.h>

#include "controller.hpp"
#include "logger.hpp"

using namespace std;

Network::Network(Controller *controller) : _controller(controller) {}

void Network::init(const string &networkSpec) {
    logger.info("Loading network specification");

    simdjson::ondemand::parser parser;
    simdjson::padded_string json = simdjson::padded_string::load(networkSpec);
    simdjson::ondemand::document root = parser.iterate(json);
    simdjson::ondemand::object obj;
    simdjson::ondemand::array arr;

    if (!root["hosts"].get(obj)) {
        for (auto field : root["hosts"].get_object()) {
            auto name = string_view(field.unescaped_key());
            auto host = make_shared<Host>(field.value().get_object(), this);

            if (!this->_nodes.emplace(name, host).second) {
                logger.error("Duplicate node " + string(name));
            }

            if (!this->_hosts.emplace(name, host).second) {
                logger.error("Duplicate host " + string(name));
            }
        }
    }

    if (!root["switches"].get(obj)) {
        for (auto field : root["switches"].get_object()) {
            auto name = string_view(field.unescaped_key());
            auto sw = make_shared<Switch>(field.value().get_object(), this);

            if (!this->_nodes.emplace(name, sw).second) {
                logger.error("Duplicate node " + string(name));
            }

            if (!this->_switches.emplace(name, sw).second) {
                logger.error("Duplicate switch " + string(name));
            }
        }
    }

    if (!root["groups"].get(obj)) {
        for (auto grField : root["groups"].get_object()) {
            auto grName = string_view(grField.unescaped_key());
            unordered_set<shared_ptr<Node>> nodes;

            for (string_view nodeName : grField.value().get_array()) {
                nodes.emplace(this->_nodes.at(string(nodeName)));
            }

            this->_nodeGroups.emplace(grName, std::move(nodes));
        }
    }

    if (!root["links"].get(arr)) {
        for (auto value : root["links"].get_array()) {
            if (!this->_links.emplace(Link(value.get_object(), this)).second) {
                logger.error("Duplicate link");
            }
        }
    }

    if (!root["rules"].get(obj)) {
        this->_rib = RIB(root["rules"].get_object());
    }
}
