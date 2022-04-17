#include <boost/program_options.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "../third-party/ThreadPool/ThreadPool.h"

#include "consumer_producer.hpp"

namespace po = boost::program_options;

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

    if (!(vm.count("url") && vm.count("depth") &&
        vm.count("network_threads") && vm.count("parser_threads") &&
        vm.count("output")))
    {
        std::cout << opt_desc << "\n";
        return 1;
    }

    ThreadPool downloaders(vm["network_threads"].as<int>());
    ThreadPool parsers(vm["parser_threads"].as<int>());

    std::queue<LinkStruct> vLinks;

    std::queue<BodyStruct> vBody;

    LinkStruct head_link(vm["url"].as<std::string>(), vm["depth"].as<int>());

    vLinks.push(LinkStruct(vm["url"].as<std::string>(),
                                vm["depth"].as<int>()));

    std::shared_ptr<std::timed_mutex> link_v_mutex =
                                      std::make_shared<std::timed_mutex>();
    std::shared_ptr<std::timed_mutex> body_v_mutex =
                                      std::make_shared<std::timed_mutex>();
    std::shared_ptr<std::timed_mutex> file_mutex =
                                      std::make_shared<std::timed_mutex>();

    std::ofstream fout;
    fout.open(vm["output"].as<std::string>());

    boost::posix_time::ptime begin_time =
        boost::posix_time::microsec_clock::local_time();

    short downloaded_num = 0;
    short parsed_num = 0;

    std::cout << "Began working" << std::endl;

    while (!(vLinks.empty() && vBody.empty() &&
           (downloaded_num - parsed_num == 0))) {
        downloaders.enqueue([&vLinks, &vBody, &link_v_mutex, &body_v_mutex,
                           &downloaded_num] {
            downloader_fun(vLinks, vBody, link_v_mutex, body_v_mutex,
                       downloaded_num);
        });

        parsers.enqueue([&vLinks, &vBody, &link_v_mutex, &body_v_mutex,
          &file_mutex, &parsed_num, &fout] {
            parser_fun(vLinks, vBody, link_v_mutex, body_v_mutex, file_mutex,
                     parsed_num, fout);
        });
    }

    fout.close();

    std::cout << "Execution time: "
              << boost::posix_time::microsec_clock::local_time() - begin_time
              << std::endl;

    return 0;
}