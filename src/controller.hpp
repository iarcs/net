#pragma once

#include <memory>
#include <string>
#include <vector>

#include "invariant.hpp"
#include "network.hpp"

struct P4Paths {
    std::string p4configPath;
    std::string p4infoPath;
};

class Controller {
private:
    Network _network;
    std::vector<std::unique_ptr<Invariant>> _invs;
    struct P4Paths _p4Paths;
    std::string _outputDir;

    Controller();
    void connectSwitches();
    void installP4Programs();
    size_t installRIB();
    size_t installEncapDecapRules();
    size_t installVerificationRules();
    static void sig_handler(int sig);
    void startProcessPacketIn();
    void waitForThreads();
    void killAllThreads();

public:
    // Disable the copy constructor and the copy assignment operator
    Controller(const Controller &) = delete;
    Controller &operator=(const Controller &) = delete;

    static Controller &get();

    const decltype(_p4Paths) &p4Paths() const { return _p4Paths; }
    const decltype(_outputDir) &outputDir() const { return _outputDir; }

    void init(const std::string &networkSpec,
              const std::string &invSpec,
              const std::string &p4configPath,
              const std::string &p4infoPath,
              const std::string &outputDir);
    void start();
};
