#pragma once

#include <string>

#include <PI/pi.h>
#include <p4/v1/p4runtime.grpc.pb.h>

class P4CounterEntry {
private:
    std::string _counterName;
    int64_t _byteCount, _pktCount;

public:
    P4CounterEntry();
    P4CounterEntry(const std::string &counterName);
    P4CounterEntry(const p4::v1::CounterEntry &entry, pi_p4info_t *);

    const decltype(_counterName) &counterName() const { return _counterName; }
    decltype(_byteCount) byteCount() const { return _byteCount; }
    decltype(_pktCount) pktCount() const { return _pktCount; }

    void counterName(const decltype(_counterName) &n) { _counterName = n; }
    void byteCount(decltype(_byteCount) c) { _byteCount = c; }
    void pktCount(decltype(_pktCount) c) { _pktCount = c; }

    p4::v1::CounterEntry protobufMsg(pi_p4info_t *) const;
};
