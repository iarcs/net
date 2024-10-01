#pragma once

#include <memory>
#include <string>

#include <simdjson.h>

#include "hash.hpp"
#include "ipv4.hpp"

using sj_object = simdjson::simdjson_result<simdjson::ondemand::object>;

class Rule {
private:
    IPNetwork _prefix;
    std::string _nhop;

    friend bool operator==(const Rule &, const Rule &) = default;

public:
    Rule(const IPNetwork &, const std::string &);
    Rule(sj_object root);

    const decltype(_prefix) &prefix() const { return _prefix; }
    const decltype(_nhop) &nhop() const { return _nhop; }
};

bool operator<(const Rule &, const Rule &);

namespace std {

template <>
struct hash<Rule> {
    size_t operator()(const Rule &rule) const {
        size_t value = 0;
        hash<IPNetwork> prefixHasher;
        hash<string> strHasher;
        value = ::hash::hash_combine(prefixHasher(rule.prefix()), value);
        value = ::hash::hash_combine(strHasher(rule.nhop()), value);
        return value;
    }
};

} // namespace std
