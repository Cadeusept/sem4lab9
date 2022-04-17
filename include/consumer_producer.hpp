// Copyright 2021 Your Name <your_email>

#ifndef INCLUDE_EXAMPLE_HPP_
#define INCLUDE_EXAMPLE_HPP_

#include "/usr/include/boost/beast/example/common/root_certificates.hpp"
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
//#include <openssl/ssl.h>
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
};

std::stringstream consumer_fun(std::string url);

static void producer_fun(GumboNode* node, const int depth);

#endif // INCLUDE_EXAMPLE_HPP_
