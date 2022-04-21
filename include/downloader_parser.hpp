// Copyright 2021 Your Name <your_email>

#ifndef INCLUDE_DOWNLOADER_PARSER_HPP_
#define INCLUDE_DOWNLOADER_PARSER_HPP_

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
#include <utility>

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
                         const std::shared_ptr<std::timed_mutex>& body_v_mutex,
                         const std::shared_ptr<std::timed_mutex>& console_mutex,
                         size_t& in_process);

[[maybe_unused]] void parser_fun(BodyStruct input,
                         std::queue<LinkStruct>& vLinks,
                         const std::shared_ptr<std::timed_mutex>& link_v_mutex,
                         const std::shared_ptr<std::timed_mutex>& file_mutex,
                         const std::shared_ptr<std::timed_mutex>& console_mutex,
                         size_t& in_process,
                         std::ofstream& fout);

std::string download_http(std::string host, std::string port,
                          const std::string& target);

std::string download_https(std::string host, std::string port,
                           const std::string& target);

#endif // INCLUDE_DOWNLOADER_PARSER_HPP_
