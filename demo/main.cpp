#include <boost/program_options.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "../third-party/ThreadPool/ThreadPool.h"

#include "consumer_producer.hpp"

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
    po::options_description opt_desc("Needed options");
    opt_desc.add_options()
        ("url", po::value<std::string>(), "set website url for parsing, ")
        ("depth", po::value<int>(), "set website parsing depth")
        ("network_threads", po::value<int>(),
            "set number of downloading threads")
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
    } else if ((unsigned int)abs(vm["network_threads"].as<int>() +
               vm["parser_threads"].as<int>()) >
               std::thread::hardware_concurrency())
        std::cout << "Max sum of threads is " <<
          std::thread::hardware_concurrency() <<
          ". Rerun program with valid data"   << std::endl;

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

    size_t in_process = 0;

    std::cout << "Began working" << std::endl;

    while (true) {
        if (!vLinks.empty()) {
            link_v_mutex->lock();
            LinkStruct data = vLinks.front();
            vLinks.pop();
            link_v_mutex->unlock();

            in_process++;
            //downloaders.enqueue(downloader_fun, data, vBody, body_v_mutex);

            downloaders.enqueue([data, &vBody, &body_v_mutex] {
                downloader_fun(data, vBody, body_v_mutex);
            });
        }

        if (!vBody.empty()) {
            body_v_mutex->lock();
            BodyStruct data = vBody.front();
            vBody.pop();
            body_v_mutex->unlock();

            //parsers.enqueue(parser_fun, data, vLinks, link_v_mutex,
            //                file_mutex, in_process, fout);

            parsers.enqueue([data, &vLinks, &link_v_mutex, &file_mutex,
                             &in_process, &fout] {
                parser_fun(data, vLinks, link_v_mutex, file_mutex,
                         in_process, fout);
            });
        }

        if (in_process == 0 && vLinks.empty() && vBody.empty()) {
            break;
        }
    }
    fout.close();

    std::cout << "Execution time: "
              << boost::posix_time::microsec_clock::local_time() - begin_time
              << std::endl;

    return 0;
}