#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <pcapplusplus/MacAddress.h>
#include <simdjson.h>

using sj_object = simdjson::simdjson_result<simdjson::ondemand::object>;

class Node;

class Interface {
private:
    Node *_parentNode;
    std::string _name;
    uint16_t _port;
    std::string _ipAddr;
    int _ipPrefixLen;
    pcpp::MacAddress _macAddr;
    std::weak_ptr<Node> _neighborNode;
    std::weak_ptr<Interface> _neighborIntf;

public:
    Interface(sj_object root, Node *parent);
    Interface(const Interface &) = delete;
    Interface(Interface &&) = default;

    void ipAddr(const std::string &addr) { _ipAddr = addr; }
    void ipPrefixLen(int len) { _ipPrefixLen = len; }
    void macAddr(const std::string &addr) { _macAddr = addr; }
    void neighborNode(const std::shared_ptr<Node> &n) { _neighborNode = n; }
    void neighborIntf(const std::shared_ptr<Interface> &i) {
        _neighborIntf = i;
    }

    decltype(_parentNode) parentNode() const { return _parentNode; }
    const decltype(_name) &name() const { return _name; }
    const decltype(_port) &port() const { return _port; }
    const decltype(_ipAddr) &ipAddr() const { return _ipAddr; }
    const decltype(_ipPrefixLen) &ipPrefixLen() const { return _ipPrefixLen; }
    const decltype(_macAddr) &macAddr() const { return _macAddr; }
    std::shared_ptr<Node> neighborNode() const { return _neighborNode.lock(); }
    std::shared_ptr<Interface> neighborIntf() const {
        return _neighborIntf.lock();
    }
};
