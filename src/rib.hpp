#pragma once

#include <memory>
#include <set>
#include <string>
#include <unordered_map>

#include <simdjson.h>

#include "ipv4.hpp"
#include "rule.hpp"

using sj_object = simdjson::simdjson_result<simdjson::ondemand::object>;

class RIB {
private:
    std::unordered_map<std::string,
                       std::unordered_map<IPNetwork, std::set<Rule>>>
        _tbl; // node name -> ( prefix -> [ rules/nhops ] )

public:
    RIB() = default;
    RIB(const RIB &) = delete;
    RIB(RIB &&) = default;
    RIB(sj_object root);

    RIB &operator=(const RIB &) = delete;
    RIB &operator=(RIB &&) = default;

    typedef decltype(_tbl)::iterator iterator;
    typedef decltype(_tbl)::const_iterator const_iterator;

    iterator begin() { return _tbl.begin(); }
    const_iterator begin() const { return _tbl.cbegin(); }
    iterator end() { return _tbl.end(); }
    const_iterator end() const { return _tbl.cend(); }
};
