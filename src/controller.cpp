#include "controller.hpp"

#include <fstream>

#include "constants.hpp"
#include "invariant.hpp"
#include "logger.hpp"
#include "util.hpp"

using namespace std;

Controller::Controller() : _network(this) {}

Controller &Controller::get() {
    static Controller instance;
    return instance;
}

void Controller::init(const string &networkSpec,
                      const string &invSpec,
                      const string &p4configPath,
                      const string &p4infoPath,
                      const string &outputDir) {
    this->_network.init(networkSpec);
    this->_invs = InvariantsParser()(invSpec);
    this->_p4Paths = {p4configPath, p4infoPath};
    this->_outputDir = outputDir;

    // TODO: Calculate the invariant-wide equivalent packet sets
    // self.ps_to_invs = dict() # PacketSet -> [ Invariants ]
    // self.ps_to_dfas = dict() # PacketSet -> DFA
}

void Controller::connectSwitches() {
    for (const auto &[name, sw] : this->_network.switches()) {
        sw->connect();
    }
}

void Controller::installP4Programs() {
    // For now we assume all P4 switches share the same P4 program/config, so we
    // read the files once and pass in the content for all switches. If the
    // assumption changes, where each switch may have a different
    // program/config, we may simply pass in the file paths.
    string p4config, p4info;

    if (this->_p4Paths.p4configPath.empty()) {
        logger.error("P4 config path empty");
    }

    ifstream ifs(this->_p4Paths.p4configPath);
    p4config =
        string(istreambuf_iterator<char>(ifs), istreambuf_iterator<char>());

    if (!this->_p4Paths.p4infoPath.empty()) {
        ifstream ifs(this->_p4Paths.p4infoPath);
        p4info =
            string(istreambuf_iterator<char>(ifs), istreambuf_iterator<char>());
    }

    for (const auto &[name, sw] : this->_network.switches()) {
        sw->installP4Config(p4config, p4info);
    }
}

size_t Controller::installRIB() {
    size_t numRules = 0;

    for (const auto &[nodeName, prefixMap] : this->_network.rib()) {
        auto it = this->_network.nodes().find(nodeName);

        if (it == this->_network.nodes().end()) {
            logger.warn(nodeName + " doesn't exist. Skipping rules");
            continue;
        }

        numRules += it->second->installRules(prefixMap);
    }

    return numRules;
}

size_t Controller::installEncapDecapRules() {
    size_t numRules = 0;

    for (const auto &[swName, sw] : this->_network.switches()) {
        // Only install at the border switches
        if (sw->hostPorts().empty()) {
            continue;
        }

        // Install rules for entering the network
        {
            P4TableEntry entry;
            entry.tableName("MyIngress.encapsulation");
            entry.addMatchField("hdr.ipv4.protocol",
                                {uint_to_be_str(PROTO_VERIFICATION),
                                 uint_to_be_str(PROTO_VERIFICATION)});
            entry.actionName("NoAction");
            entry.priority(Priority::High);
            sw->installTableEntry(entry);
        }
        {
            P4TableEntry entry;
            entry.tableName("MyIngress.encapsulation");
            entry.actionName("MyIngress.insert_verification_header");
            entry.priority(Priority::Low);
            sw->installTableEntry(entry);
        }

        // Install rules for leaving the network
        for (const auto &hostPort : sw->hostPorts()) {
            P4TableEntry entry;
            entry.tableName("MyEgress.check_leaving");
            entry.addMatchField("std_meta.egress_port",
                                {uint_to_be_str(hostPort)});
            entry.actionName("MyEgress.mark_leaving");
            sw->installTableEntry(entry);
        }

        // Install rules for removing verification headers
        {
            P4TableEntry entry;
            entry.tableName("MyEgress.decapsulation");
            entry.addMatchField("meta.verification.leaving",
                                {uint_to_be_str(uint8_t(1))});
            entry.actionName("MyEgress.remove_verification_header");
            sw->installTableEntry(entry);
        }

        numRules += 3 + sw->hostPorts().size();
    }

    return numRules;
}

size_t Controller::installVerificationRules() {
    size_t numRules = 0;

    if (!this->_invs.empty()) {
        numRules += this->installEncapDecapRules();
    }

    // Compile rules
    unordered_map<shared_ptr<Switch>, unordered_set<P4TableEntry>> entriesMap;

    for (const auto &inv : this->_invs) {
        auto em = inv->getTableEntries(this->_network);
        entriesMap.merge(em);
        for (auto &[sw, entries] : em) {
            entriesMap.at(sw).merge(entries);
        }
    }

    // Install rules
    for (auto &[sw, entries] : entriesMap) {
        for (auto &entry : entries) {
            sw->installTableEntry(entry);
        }

        numRules += entries.size();
    }

    return numRules;
}

static const int sigs[] = {SIGHUP, SIGINT, SIGQUIT, SIGTERM};

void Controller::sig_handler(int sig) {
    switch (sig) {
    case SIGHUP:
    case SIGINT:
    case SIGQUIT:
    case SIGTERM: {
        Controller &controller = Controller::get();
        controller.killAllThreads();
        controller.waitForThreads();
        logger.info("Terminated");
        exit(0);
    }
    }
}

void Controller::startProcessPacketIn() {
    // Process each switch asynchronously
    for (const auto &[_, sw] : this->_network.switches()) {
        sw->startProcessPacketIn();
    }

    // Register signal handler
    struct sigaction action;
    action.sa_handler = Controller::sig_handler;
    sigemptyset(&action.sa_mask);
    for (size_t i = 0; i < sizeof(sigs) / sizeof(int); ++i) {
        sigaddset(&action.sa_mask, sigs[i]);
    }
    for (size_t i = 0; i < sizeof(sigs) / sizeof(int); ++i) {
        sigaction(sigs[i], &action, nullptr);
    }
}

void Controller::waitForThreads() {
    for (const auto &[_, sw] : this->_network.switches()) {
        sw->waitForThreads();
    }
}

void Controller::killAllThreads() {
    for (const auto &[_, sw] : this->_network.switches()) {
        sw->killAllThreads();
    }
}

void Controller::start() {

    // TODO: set_reduced_MTU for end hosts
    // if len(self.invariants) > 0:
    //     for host_dict in self.network['hosts'].values():
    //         for intf in host_dict['intfs'].keys():
    //             # MTU: 1500 - verification header length
    //             cmd = 'ip link set dev {} mtu 1497'.format(intf)
    //             run_mnexc_cmd(host_dict['pid'], cmd)

    logger.info("Connect to network");
    this->connectSwitches();

    logger.info("Install P4 programs");
    this->installP4Programs();

    size_t fwdRules = this->installRIB();
    logger.info("Install forwarding rules... " + to_string(fwdRules));

    size_t verRules = this->installVerificationRules();
    logger.info("Install verification rules... " + to_string(verRules));

    logger.info("===========================================");
    logger.info("Start controller-switch event loops");
    logger.info("===========================================");
    this->startProcessPacketIn();
    this->waitForThreads();
}
