#include "interface.hpp"

#include "logger.hpp"
#include "node.hpp"

using namespace std;
using simdjson::ondemand::json_type;

Interface::Interface(sj_object root, Node *parent)
    : _parentNode(parent), _ipPrefixLen(-1) {
    this->_name = string_view(root["name"].get_string());
    this->_port = root["port"].get_uint64();

    simdjson::ondemand::raw_json_string str;
    simdjson::ondemand::number num;

    if (!root["ip"].get(str)) {
        this->_ipAddr = string_view(root["ip"].get_string());
    }

    if (!root["prefixLen"].get(str) || !root["prefixLen"].get(num)) {
        json_type type = root["prefixLen"].type();

        if (type == json_type::string) {
            this->_ipPrefixLen =
                stoi(string(string_view(root["prefixLen"].get_string())));
        } else if (type == json_type::number) {
            this->_ipPrefixLen = root["prefixLen"].get_uint64();
        } else {
            logger.error("Invalid type");
        }
    }

    if (!root["mac"].get(str)) {
        this->_macAddr = string(string_view(root["mac"].get_string()));
    }
}
