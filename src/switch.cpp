#include "switch.hpp"

#include <boost/filesystem.hpp>

#include "controller.hpp"
#include "network.hpp"
#include "util.hpp"

using namespace std;
namespace fs = boost::filesystem;

Switch::Switch(sj_object root, Network *network) : Node(root, network) {
    this->_deviceId = root["device_id"].get_uint64();
    this->_grpcPort = root["grpc_port"].get_uint64();

    for (uint64_t port : root["host_ports"].get_array()) {
        this->_hostPorts.insert(port);
    }

    this->_p4 = make_unique<P4RuntimeMgr>(this);
}

size_t
Switch::installRules(const unordered_map<IPNetwork, set<Rule>> &prefixMap) {
    size_t numRules = 0;
    uint8_t ecmpGroupId = 1; // bit<8>

    for (const auto &[prefix, rules] : prefixMap) {
        if (rules.size() == 1) {
            // IPv4 forwarding
            const Rule &rule = *rules.begin();
            auto srcIntfs = this->getIntfsByNeighborNode(rule.nhop());

            if (srcIntfs == nullptr || srcIntfs->empty()) {
                logger.warn("No interface of " + this->_name +
                            " is connected to " + rule.nhop());
                continue;
            }

            for (const auto &srcIntf : *srcIntfs) {
                const auto &dstIntf = srcIntf->neighborIntf();
                assert(dstIntf != nullptr);

                P4TableEntry entry;
                entry.tableName("MyIngress.ipv4_forwarding.ipv4_lpm");
                entry.addMatchField(
                    "hdr.ipv4.dstAddr",
                    {uint_to_be_str(prefix.networkAddr().value()),
                     to_string(prefix.prefix())});
                entry.actionName("MyIngress.ipv4_forwarding.ipv4_forward");
                entry.addActionParam("dstAddr",
                                     mac_to_be_str(dstIntf->macAddr()));
                entry.addActionParam("port", uint_to_be_str(srcIntf->port()));
                this->installTableEntry(entry);
            }

            numRules += srcIntfs->size();
        } else if (rules.size() > 1) {
            // ECMP

            // ecmp_select
            {
                P4TableEntry entry;
                entry.tableName("MyIngress.ipv4_forwarding.ipv4_lpm");
                if (prefix.prefix() > 0) {
                    entry.addMatchField(
                        "hdr.ipv4.dstAddr",
                        {uint_to_be_str(prefix.networkAddr().value()),
                         to_string(prefix.prefix())});
                }
                entry.actionName("MyIngress.ipv4_forwarding.ecmp_select");
                entry.addActionParam("ecmp_group_id",
                                     uint_to_be_str(ecmpGroupId));
                entry.addActionParam(
                    "ecmp_count",
                    uint_to_be_str(static_cast<uint8_t>(rules.size())));
                this->installTableEntry(entry);
            }

            numRules++;
            uint8_t selectId = 0;

            for (const auto &rule : rules) {
                auto srcIntfs = this->getIntfsByNeighborNode(rule.nhop());

                if (srcIntfs == nullptr || srcIntfs->empty()) {
                    logger.warn("No interface of " + this->_name +
                                " is connected to " + rule.nhop());
                    continue;
                }

                for (const auto &srcIntf : *srcIntfs) {
                    const auto &dstIntf = srcIntf->neighborIntf();
                    assert(dstIntf != nullptr);

                    // ecmp_forward
                    P4TableEntry entry;
                    entry.tableName("MyIngress.ipv4_forwarding.ecmp_forward");
                    entry.addMatchField("meta.ecmp_group_id",
                                        {uint_to_be_str(ecmpGroupId)});
                    entry.addMatchField("meta.ecmp_select",
                                        {uint_to_be_str(selectId)});
                    entry.actionName("MyIngress.ipv4_forwarding.ipv4_forward");
                    entry.addActionParam("dstAddr",
                                         mac_to_be_str(dstIntf->macAddr()));
                    entry.addActionParam("port",
                                         uint_to_be_str(srcIntf->port()));
                    this->installTableEntry(entry);
                }

                numRules += srcIntfs->size();
                selectId++;
            }

            ecmpGroupId++;
        }
    }

    return numRules;
}

void Switch::connect() {
    this->_p4->connect();

    // Enable per-switch logging for packet handler threads to avoid messed up
    // output with multiple threads
    auto logFn = fs::path(this->_network->controller()->outputDir()) /
                 ("logs/" + this->_name + ".log");
    this->_swLogger = make_unique<Logger>(this->_name);
    this->_swLogger->enable_file_logging(logFn.string(), true /* append */);
}

void Switch::installP4Config(const string &p4config, const string &p4info) {
    this->_p4->setForwardingPipelineConfig(p4config, p4info);
}

void Switch::installTableEntry(const P4TableEntry &entry) {
    this->_p4->writeTableEntry(entry);
}

void Switch::startProcessPacketIn() {
    this->_p4->startProcessPacketIn();
}

void Switch::waitForThreads() {
    this->_p4->waitForThreads();
}
void Switch::killAllThreads() {
    this->_p4->killAllThreads();
}
