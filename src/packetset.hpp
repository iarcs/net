#pragma once

#include <string>
#include <vector>

#include <simdjson.h>

using sj_object = simdjson::simdjson_result<simdjson::ondemand::object>;

class PacketSet {
private:
    ;

public:
    PacketSet();
    PacketSet(sj_object root);

    std::vector<std::string> srcIPRange() const;
    std::vector<std::string> dstIPRange() const;
};
