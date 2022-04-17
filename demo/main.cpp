#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/config.hpp>
#include <boost/program_options.hpp>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <gumbo.h>
#include "../third-party/ThreadPool/ThreadPool.h"

#include "consumer_producer.hpp"

namespace po = boost::program_options;

std::vector<LinkStruct> vLinks;

int main(int argc, char* argv[]) {
    po::options_description opt_desc("Needed options");
    opt_desc.add_options()
        ("url", po::value<std::string>(), "set website url for parsing, ")
        ("depth", po::value<int>(), "set website parsing depth")
        ("network_threads", po::value<int>(), "set number of downloading threads")
        ("parser_threads", po::value<int>(), "set number of parsing threads")
        ("output", po::value<std::string>(), "set output file path")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, opt_desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << opt_desc << "\n";
        return 1;
    }

    if (vm.count("url") && vm.count("depth") &&
        vm.count("network_threads") && vm.count("parser_threads") &&
        vm.count("output")) {
        ThreadPool downloaders(vm["network_threads"].as<int>());
        ThreadPool parsers(vm["parser_threads"].as<int>());
    } else {
        std::cout << opt_desc << "\n";
        return 1;
    }

    LinkStruct head_link(vm["url"].as<std::string>(), vm["depth"].as<int>());

    vLinks.push_back(vm["url"].as<std::string>());


    return 0;
}