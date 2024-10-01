#pragma once

#include <memory>
#include <string>
#include <vector>

#include <PI/pi.h>
#include <google/protobuf/message.h>
#include <p4/v1/p4runtime.grpc.pb.h>

#include "logger.hpp"
#include "p4counterentry.hpp"
#include "p4tableentry.hpp"
#include "streamclient.hpp"

class Switch;

/**
 * https://github.com/p4lang/p4runtime/blob/main/proto/p4/v1/p4runtime.proto
 */
class P4RuntimeMgr {
private:
    const Switch *_sw;
    pi_p4info_t *_p4info;
    std::unique_ptr<p4::v1::P4Runtime::Stub> _piStub;
    std::unique_ptr<StreamClient> _streamClient;
    std::unique_ptr<Logger> _p4rtLogger;

    void logMessage(const google::protobuf::Message &msg);

public:
    P4RuntimeMgr(const Switch *sw);
    ~P4RuntimeMgr();

    decltype(_sw) sw() const { return _sw; }

    void connect();
    void startProcessPacketIn();
    void waitForThreads();
    void killAllThreads();
    void setForwardingPipelineConfig(const std::string &p4config,
                                     const std::string &p4info);
    void writeTableEntry(const P4TableEntry &);
    void deleteTableEntry(const P4TableEntry &);
    std::vector<P4TableEntry> readTableEntries();
    std::vector<P4CounterEntry> readCounters();
    // void readDirectCounter();
    // void readRegister();
    // void packetOut();
    // void packetIn();
};
