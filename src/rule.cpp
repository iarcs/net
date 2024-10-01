#include "rule.hpp"

#include "logger.hpp"

using namespace std;

Rule::Rule(const IPNetwork &prefix, const string &nhop)
    : _prefix(prefix), _nhop(nhop) {}

Rule::Rule(sj_object root) {

    for (auto field : root) {
        auto key = string_view(field.unescaped_key());
        auto value = string_view(field.value().get_string());

        if (key == "prefix") {
            this->_prefix = string(value);
        } else if (key == "next_hop") {
            this->_nhop = value;
        } else {
            logger.error("Unknown rule key: " + string(key));
        }

        // Assuming all fields are specified, exception handling may be added
        // later.
    }
}

bool operator<(const Rule &a, const Rule &b) {
    if (a.prefix().networkAddr() < b.prefix().networkAddr()) {
        return true;
    } else if (a.prefix().networkAddr() > b.prefix().networkAddr()) {
        return false;
    }

    if (a.prefix().prefix() < b.prefix().prefix()) {
        return true;
    } else if (a.prefix().prefix() > b.prefix().prefix()) {
        return false;
    }

    return a.nhop() < b.nhop();
}
