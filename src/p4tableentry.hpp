#pragma once

#include <map>
#include <string>
#include <vector>

#include <PI/pi.h>
#include <p4/v1/p4runtime.grpc.pb.h>

#include "hash.hpp"
#include "priority.hpp"

class P4TableEntry {
private:
    std::string _tableName;
    std::map<std::string, std::vector<std::string>> _matchFields;
    std::string _actionName;
    std::map<std::string, std::string> _actionParams;
    int _priority = 0;

    friend bool operator==(const P4TableEntry &,
                           const P4TableEntry &) = default;

public:
    P4TableEntry() = default;
    P4TableEntry(const std::string &tableName, const std::string &actionName);
    P4TableEntry(const p4::v1::TableEntry &entry, pi_p4info_t *);

    const decltype(_tableName) &tableName() const { return _tableName; }
    const decltype(_matchFields) &matchFields() const { return _matchFields; }
    const decltype(_actionName) &actionName() const { return _actionName; }
    const decltype(_actionParams) &actionParams() const {
        return _actionParams;
    }
    decltype(_priority) priority() const { return _priority; }

    void tableName(const decltype(_tableName) &n) { _tableName = n; }
    void addMatchField(const std::string &name,
                       std::vector<std::string> &&values);
    void actionName(const decltype(_actionName) &n) { _actionName = n; }
    void addActionParam(const std::string &name, const std::string &value);
    void priority(decltype(_priority) p) { _priority = p; }
    void priority(Priority p) { _priority = static_cast<int>(p); }

    p4::v1::TableEntry protobufMsg(pi_p4info_t *) const;
};

namespace std {

template <>
struct hash<P4TableEntry> {
    size_t operator()(const P4TableEntry &rule) const {
        size_t value = 0;

        auto strHasher = [](const string &s, size_t seed) {
            return ::hash::hash(s.c_str(), s.size(), seed);
        };

        value = strHasher(rule.tableName(), value);
        for (const auto &[mfName, mfValues] : rule.matchFields()) {
            value = strHasher(mfName, value);
            for (const auto &mfValue : mfValues) {
                value = strHasher(mfValue, value);
            }
        }
        value = strHasher(rule.actionName(), value);
        for (const auto &[apName, apValue] : rule.actionParams()) {
            value = strHasher(apName, value);
            value = strHasher(apValue, value);
        }
        value = ::hash::hash_combine(hash<int>()(rule.priority()), value);

        return value;
    }
};

} // namespace std
