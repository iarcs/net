#include "host.hpp"

#include "network.hpp"

using namespace std;

Host::Host(sj_object root, Network *network) : Node(root, network) {
    this->_hostId = root["host_id"].get_uint64();
}

size_t Host::installRules(const unordered_map<IPNetwork, set<Rule>> &prefixMap
                          __attribute__((unused))) {
    logger.warn("Host::installRules is not implemented yet");
    return 0;
}
