#pragma once

#include <string>

class Controller;
class Switch;
class P4RuntimeMgr;

class PacketHandler {
private:
    const Controller *_controller;
    const Switch *_switch;
    const P4RuntimeMgr *_p4rtmanager;
    int _invId;
    std::string _frame;

public:
    PacketHandler(const Controller *controller,
                  const Switch *sw,
                  const P4RuntimeMgr *p4rtmanager,
                  std::string &&invId,
                  std::string &&frame);

    void operator()();
};
