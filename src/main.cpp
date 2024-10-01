#include <exception>
#include <iostream>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include "controller.hpp"
#include "logger.hpp"

using namespace std;
namespace po = boost::program_options;
namespace fs = boost::filesystem;

int main(int argc, char **argv) {
    po::options_description desc("Controller options");
    desc.add_options()("help,h", "Show help message");
    desc.add_options()("output,o", po::value<string>(), "Output directory");
    desc.add_options()("network", po::value<string>(), "Network specification");
    desc.add_options()("invariants", po::value<string>(),
                       "Invariants specification");
    desc.add_options()("p4", po::value<string>(), "P4 config json");
    desc.add_options()("p4info", po::value<string>(),
                       "P4Info proto in text format");
    po::variables_map vm;

    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch (const exception &e) {
        cerr << e.what() << endl;
        return 1;
    }

    if (vm.count("help") > 0) {
        cout << desc << endl;
        return 0;
    }

    if (vm.count("output") == 0) {
        cerr << "Missing output directory" << endl << desc << endl;
        return 1;
    }

    fs::path outputDir = vm.at("output").as<string>();
    fs::create_directories(outputDir);
    logger.enable_console_logging();
    logger.enable_file_logging((outputDir / "controller.log").string());

    Controller &controller = Controller::get();
    controller.init(vm.at("network").as<string>(),
                    vm.at("invariants").as<string>(), vm.at("p4").as<string>(),
                    vm.at("p4info").as<string>(), vm.at("output").as<string>());
    controller.start();
    return 0;
}
