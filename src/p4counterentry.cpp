#include "p4counterentry.hpp"

#include "logger.hpp"

using namespace std;

P4CounterEntry::P4CounterEntry() : _byteCount(0), _pktCount(0) {}

P4CounterEntry::P4CounterEntry(const std::string &counterName)
    : _counterName(counterName), _byteCount(0), _pktCount(0) {}

P4CounterEntry::P4CounterEntry(const p4::v1::CounterEntry &entry,
                               pi_p4info_t *p4info) {
    this->_counterName =
        pi_p4info_counter_name_from_id(p4info, entry.counter_id());
    this->_byteCount = entry.data().byte_count();
    this->_pktCount = entry.data().packet_count();
}

p4::v1::CounterEntry P4CounterEntry::protobufMsg(pi_p4info_t *p4info) const {
    p4::v1::CounterEntry entry;

    pi_p4_id_t counterId =
        pi_p4info_counter_id_from_name(p4info, this->_counterName.c_str());
    entry.set_counter_id(counterId);

    auto data = entry.mutable_data();
    data->set_byte_count(this->_byteCount);
    data->set_packet_count(this->_pktCount);

    return entry;
}
