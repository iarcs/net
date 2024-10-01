#pragma once

#include <cmath>
#include <cstdio>
#include <functional>
#include <string>

#include "hash.hpp"
#include "logger.hpp"

class IPAddress {
protected:
    uint32_t _value;

    friend bool operator<(const IPAddress &, const IPAddress &);
    friend bool operator<=(const IPAddress &, const IPAddress &);
    friend bool operator>(const IPAddress &, const IPAddress &);
    friend bool operator>=(const IPAddress &, const IPAddress &);
    friend bool operator==(const IPAddress &, const IPAddress &);
    friend bool operator!=(const IPAddress &, const IPAddress &);

public:
    IPAddress() = default;
    IPAddress(const IPAddress &) = default;
    IPAddress(IPAddress &&) = default;
    IPAddress(const std::string &);
    IPAddress(const char *);
    IPAddress(uint32_t);

    std::string toString() const;
    size_t length() const;
    uint32_t value() const;
    IPAddress &operator=(const IPAddress &) = default;
    IPAddress &operator=(IPAddress &&) = default;
    IPAddress &operator+=(const IPAddress &);
    IPAddress &operator-=(const IPAddress &);
    IPAddress &operator&=(const IPAddress &);
    IPAddress &operator|=(const IPAddress &);
    IPAddress operator+(const IPAddress &) const;
    int operator-(const IPAddress &) const;
    IPAddress operator&(const IPAddress &) const;
    uint32_t operator&(uint32_t) const;
    IPAddress operator|(const IPAddress &) const;
    uint32_t operator|(uint32_t) const;
};

bool operator<(const IPAddress &, const IPAddress &);
bool operator<=(const IPAddress &, const IPAddress &);
bool operator>(const IPAddress &, const IPAddress &);
bool operator>=(const IPAddress &, const IPAddress &);
bool operator==(const IPAddress &, const IPAddress &);
bool operator!=(const IPAddress &, const IPAddress &);

class IPNetwork;

class IPInterface : protected IPAddress {
protected:
    int _prefix; // number of bits

public:
    IPInterface() = default;
    IPInterface(const IPInterface &) = default;
    IPInterface(IPInterface &&) = default;
    IPInterface(const IPAddress &, int);
    IPInterface(const std::string &);
    IPInterface(const char *);

    std::string toString() const;
    int prefix() const;
    IPAddress addr() const;
    IPAddress mask() const;
    IPNetwork network() const;
    bool operator==(const IPInterface &) const;
    bool operator!=(const IPInterface &) const;
    IPInterface &operator=(const IPInterface &) = default;
    IPInterface &operator=(IPInterface &&) = default;
};

bool operator<(const IPInterface &, const IPInterface &);

class IPRange;

class IPNetwork : public IPInterface {
private:
    bool isNetwork() const; // check if it's the network address

public:
    IPNetwork() = default;
    IPNetwork(const IPNetwork &) = default;
    IPNetwork(IPNetwork &&) = default;
    IPNetwork(const IPAddress &, int);
    IPNetwork(const std::string &);
    IPNetwork(const char *);
    IPNetwork(const IPRange &);

    IPAddress networkAddr() const;
    IPAddress broadcastAddr() const;
    bool contains(const IPAddress &) const;
    bool contains(const IPRange &) const;
    IPRange range() const;
    bool operator==(const IPNetwork &) const;
    bool operator!=(const IPNetwork &) const;
    IPNetwork &operator=(const IPNetwork &) = default;
    IPNetwork &operator=(IPNetwork &&) = default;
};

class IPRange {
protected:
    IPAddress _lb;
    IPAddress _ub;

public:
    IPRange() = default;
    IPRange(const IPRange &) = default;
    IPRange(IPRange &&) = default;
    IPRange(const IPAddress &lb, const IPAddress &ub);
    IPRange(const IPNetwork &);
    IPRange(const std::string &);
    IPRange(const char *);

    std::string toString() const;
    size_t size() const;
    size_t length() const;
    void lb(const IPAddress &);
    void ub(const IPAddress &);
    IPAddress lb() const;
    IPAddress ub() const;
    bool contains(const IPAddress &) const;
    bool contains(const IPRange &) const;
    bool isNetwork() const;
    IPNetwork network() const;
    bool operator==(const IPRange &) const;
    bool operator!=(const IPRange &) const;
    IPRange &operator=(const IPRange &) = default;
    IPRange &operator=(IPRange &&) = default;
};

//==============================================================================

namespace std {

template <>
struct hash<IPAddress> {
    size_t operator()(const IPAddress &addr) const {
        return hash<uint32_t>()(addr.value());
    }
};

template <>
struct hash<IPInterface> {
    size_t operator()(const IPInterface &intf) const {
        return ::hash::hash_combine(hash<IPAddress>()(intf.addr()),
                                    hash<int>()(intf.prefix()));
    }
};

template <>
struct hash<IPNetwork> {
    size_t operator()(const IPNetwork &net) const {
        return hash<IPInterface>()(net);
    }
};

} // namespace std
