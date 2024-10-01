#pragma once

#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <unordered_set>

#include <simdjson.h>

#include "logger.hpp"
#include "node.hpp"
#include "p4runtimemgr.hpp"
#include "p4tableentry.hpp"

using sj_object = simdjson::simdjson_result<simdjson::ondemand::object>;

class Network;

class Switch : public Node {
private:
    int _deviceId;
    int _grpcPort;
    std::unordered_set<uint16_t> _hostPorts;
    std::unique_ptr<P4RuntimeMgr> _p4;
    std::unique_ptr<Logger> _swLogger; // append to the same log file as bmv2

public:
    Switch(sj_object root, Network *network);
    ~Switch() override = default;

    decltype(_deviceId) deviceId() const { return _deviceId; }
    decltype(_grpcPort) grpcPort() const { return _grpcPort; }
    const decltype(_hostPorts) &hostPorts() const { return _hostPorts; }
    const decltype(_swLogger) &swLogger() const { return _swLogger; }

    size_t installRules(
        const std::unordered_map<IPNetwork, std::set<Rule>> &) override;

    // connect to the switch's P4 server
    void connect();
    void installP4Config(const std::string &p4config,
                         const std::string &p4info);
    void installTableEntry(const P4TableEntry &);
    void startProcessPacketIn();
    void waitForThreads();
    void killAllThreads();
};
