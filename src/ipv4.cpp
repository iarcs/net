#include "ipv4.hpp"

#include <cstdio>

#include "logger.hpp"

using namespace std;

IPAddress::IPAddress(const string &ips) {
    int parsed, r, oct[4];
    r = sscanf(ips.c_str(), "%d.%d.%d.%d%n", oct, oct + 1, oct + 2, oct + 3,
               &parsed);
    if (r != 4 || parsed != (int)ips.size()) {
        logger.error("Failed to parse IP: " + ips);
    }
    this->_value = 0;
    for (int i = 0; i < 4; ++i) {
        if (oct[i] < 0 || oct[i] > 255) {
            logger.error("Invalid IP octet: " + to_string(oct[i]));
        }
        this->_value = (this->_value << 8) + oct[i];
    }
}

IPAddress::IPAddress(const char *ips) : IPAddress(string(ips)) {}

IPAddress::IPAddress(uint32_t ip) : _value(ip) {}

string IPAddress::toString() const {
    return to_string((this->_value >> 24) & 255) + "." +
           to_string((this->_value >> 16) & 255) + "." +
           to_string((this->_value >> 8) & 255) + "." +
           to_string((this->_value) & 255);
}

size_t IPAddress::length() const {
    return 32;
}

uint32_t IPAddress::value() const {
    return this->_value;
}

IPAddress &IPAddress::operator+=(const IPAddress &rhs) {
    this->_value += rhs._value;
    return *this;
}

IPAddress &IPAddress::operator-=(const IPAddress &rhs) {
    this->_value -= rhs._value;
    return *this;
}

IPAddress &IPAddress::operator&=(const IPAddress &rhs) {
    this->_value &= rhs._value;
    return *this;
}

IPAddress &IPAddress::operator|=(const IPAddress &rhs) {
    this->_value |= rhs._value;
    return *this;
}

IPAddress IPAddress::operator+(const IPAddress &rhs) const {
    return IPAddress(this->_value + rhs._value);
}

int IPAddress::operator-(const IPAddress &rhs) const {
    return (int)(this->_value - rhs._value);
}

IPAddress IPAddress::operator&(const IPAddress &rhs) const {
    return IPAddress(this->_value & rhs._value);
}

uint32_t IPAddress::operator&(uint32_t rhs) const {
    return this->_value & rhs;
}

IPAddress IPAddress::operator|(const IPAddress &rhs) const {
    return IPAddress(this->_value | rhs._value);
}

uint32_t IPAddress::operator|(uint32_t rhs) const {
    return this->_value | rhs;
}

bool operator<(const IPAddress &a, const IPAddress &b) {
    return a._value < b._value;
}

bool operator<=(const IPAddress &a, const IPAddress &b) {
    return a._value <= b._value;
}

bool operator>(const IPAddress &a, const IPAddress &b) {
    return a._value > b._value;
}

bool operator>=(const IPAddress &a, const IPAddress &b) {
    return a._value >= b._value;
}

bool operator==(const IPAddress &a, const IPAddress &b) {
    return a._value == b._value;
}

bool operator!=(const IPAddress &a, const IPAddress &b) {
    return a._value != b._value;
}

IPInterface::IPInterface(const IPAddress &addr, int prefix)
    : IPAddress(addr), _prefix(prefix) {
    if (_prefix < 0 || (size_t)_prefix > this->length()) {
        logger.error("Invalid prefix length: " + to_string(_prefix));
    }
}

IPInterface::IPInterface(const string &cidr) {
    int r, prefix_len, parsed, oct[4];

    r = sscanf(cidr.c_str(), "%d.%d.%d.%d/%d%n", oct, oct + 1, oct + 2, oct + 3,
               &prefix_len, &parsed);
    if (r != 5 || parsed != (int)cidr.size()) {
        logger.error("Failed to parse IP: " + cidr);
    }
    this->_value = 0;
    for (int i = 0; i < 4; ++i) {
        if (oct[i] < 0 || oct[i] > 255) {
            logger.error("Invalid IP octet: " + to_string(oct[i]));
        }
        this->_value = (this->_value << 8) + oct[i];
    }
    _prefix = prefix_len;
    if (_prefix < 0 || (size_t)_prefix > this->length()) {
        logger.error("Invalid prefix length: " + to_string(_prefix));
    }
}

IPInterface::IPInterface(const char *cidr) : IPInterface(string(cidr)) {}

string IPInterface::toString() const {
    return IPAddress::toString() + "/" + to_string(_prefix);
}

int IPInterface::prefix() const {
    return this->_prefix;
}

IPAddress IPInterface::addr() const {
    return IPAddress(this->_value);
}

IPAddress IPInterface::mask() const {
    uint32_t mask = ~((1ULL << (this->length() - _prefix)) - 1);
    return IPAddress(mask);
}

IPNetwork IPInterface::network() const {
    uint32_t mask = (1ULL << (this->length() - _prefix)) - 1;
    return IPNetwork(this->_value & (~mask), _prefix);
}

bool IPInterface::operator==(const IPInterface &rhs) const {
    return (this->_value == rhs._value && _prefix == rhs._prefix);
}

bool IPInterface::operator!=(const IPInterface &rhs) const {
    return (this->_value != rhs._value || _prefix != rhs._prefix);
}

bool operator<(const IPInterface &lhs, const IPInterface &rhs) {
    if (lhs.prefix() > rhs.prefix()) {
        return true;
    } else if (lhs.prefix() < rhs.prefix()) {
        return false;
    }
    return lhs.addr() < rhs.addr();
}

bool IPNetwork::isNetwork() const {
    uint32_t mask = (1ULL << (this->length() - this->_prefix)) - 1;
    return ((this->_value & mask) == 0);
}

IPNetwork::IPNetwork(const IPAddress &addr, int prefix)
    : IPInterface(addr, prefix) {
    if (!isNetwork()) {
        logger.error("Invalid network: " + IPInterface::toString());
    }
}

IPNetwork::IPNetwork(const string &cidr) : IPInterface(cidr) {
    if (!isNetwork()) {
        logger.error("Invalid network: " + IPInterface::toString());
    }
}

IPNetwork::IPNetwork(const char *cidr) : IPNetwork(string(cidr)) {}

IPNetwork::IPNetwork(const IPRange &range) {
    uint32_t size = range.size();

    if ((size & (size - 1)) != 0 || (range.lb() & (size - 1)) != 0) {
        logger.error("Invalid network: " + range.toString());
    }

    this->_value = range.lb().value();
    this->_prefix = this->length() - (int)log2(size);
}

IPAddress IPNetwork::networkAddr() const {
    return this->addr();
}

IPAddress IPNetwork::broadcastAddr() const {
    uint32_t mask = (1ULL << (this->length() - this->_prefix)) - 1;
    return IPAddress(this->_value | mask);
}

bool IPNetwork::contains(const IPAddress &addr) const {
    return (addr >= networkAddr() && addr <= broadcastAddr());
}

bool IPNetwork::contains(const IPRange &range) const {
    return (range.lb() >= networkAddr() && range.ub() <= broadcastAddr());
}

IPRange IPNetwork::range() const {
    return IPRange(*this);
}

bool IPNetwork::operator==(const IPNetwork &rhs) const {
    return IPInterface::operator==(rhs);
}

bool IPNetwork::operator!=(const IPNetwork &rhs) const {
    return IPInterface::operator!=(rhs);
}

IPRange::IPRange(const IPAddress &lb, const IPAddress &ub) : _lb(lb), _ub(ub) {
    if (_lb > _ub) {
        logger.error("Invalid IP range: " + toString());
    }
}

IPRange::IPRange(const IPNetwork &net)
    : _lb(net.networkAddr()), _ub(net.broadcastAddr()) {}

IPRange::IPRange(const string &net) : IPRange(IPNetwork(net)) {}

IPRange::IPRange(const char *net) : IPRange(IPNetwork(net)) {}

string IPRange::toString() const {
    return "[" + _lb.toString() + ", " + _ub.toString() + "]";
}

size_t IPRange::size() const {
    return _ub - _lb + 1;
}

size_t IPRange::length() const {
    return size();
}

void IPRange::lb(const IPAddress &addr) {
    _lb = addr;
}

void IPRange::ub(const IPAddress &addr) {
    _ub = addr;
}

IPAddress IPRange::lb() const {
    return _lb;
}

IPAddress IPRange::ub() const {
    return _ub;
}

bool IPRange::contains(const IPAddress &addr) const {
    return (addr >= _lb && addr <= _ub);
}

bool IPRange::contains(const IPRange &range) const {
    return (range._lb >= _lb && range._ub <= _ub);
}

bool IPRange::isNetwork() const {
    uint32_t n = size();
    return ((n & (n - 1)) == 0 && (_lb & (n - 1)) == 0);
}

IPNetwork IPRange::network() const {
    return IPNetwork(*this);
}

bool IPRange::operator==(const IPRange &rhs) const {
    return (_lb == rhs._lb && _ub == rhs._ub);
}

bool IPRange::operator!=(const IPRange &rhs) const {
    return (_lb != rhs._lb || _ub != rhs._ub);
}
