// Copyright 2021 Your Name <your_email>

#ifndef INCLUDE_EXAMPLE_HPP_
#define INCLUDE_EXAMPLE_HPP_

#include "../third-party/root_certificates.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/config.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/program_options.hpp>
#include <queue>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <gumbo.h>
#include <cstdlib>
#include <fstream>
#include <sstream>

struct LinkStruct {
 private:
    std::string _url;
    int _depth;
 public:
    LinkStruct(std::string url, int depth) {
        _url = std::move(url);
        _depth = std::move(depth);
    }

    std::string get_url() {
        return _url;
    }

    int get_depth() {
        return _depth;
    }
};

struct BodyStruct {
 private:
    std::string _body;
    int _depth;
 public:
    BodyStruct(std::string url, int depth) {
        _body = std::move(url);
        _depth = std::move(depth);
    }

    std::string get_body() {
        return _body;
    }

    int get_depth() {
        return _depth;
    }
};

[[maybe_unused]] void downloader_fun(LinkStruct input,
                         std::queue<BodyStruct>& vBody,
                         const std::shared_ptr<std::timed_mutex>& body_v_mutex);

[[maybe_unused]] void parser_fun(BodyStruct input,
                         std::queue<LinkStruct>& vLinks,
                         const std::shared_ptr<std::timed_mutex>&
                             link_v_mutex,
                         const std::shared_ptr<std::timed_mutex>& file_mutex,
                         size_t* parsed_num,
                         std::ofstream& fout);

#endif // INCLUDE_EXAMPLE_HPP_
