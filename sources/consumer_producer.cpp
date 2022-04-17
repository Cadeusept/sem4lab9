// Copyright 2021 Your Name <your_email>

#include <consumer_producer.hpp>
#include <queue>
#include <stdexcept>

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

// Performs an HTTP GET and prints the response
std::stringstream consumer_fun(const std::string url)
{
    std::stringstream ss;
    try
    {
        "Usage: http-client-sync-ssl <host> <port> <target> [<HTTP version: 1.0 or 1.1(default)>]\n"
        "Example:\n"
        "    http-client-sync-ssl www.example.com 443 /\n"
        "    http-client-sync-ssl www.example.com 443 / 1.0\n";

        auto port = "80";
        const int version = 11; //http version
        std::string tmp;

        // адрес страницы для скачивания: https://www.boost.org/doc/libs/1_69_0/libs/beast/example/http/client/sync-ssl/http_client_sync_ssl.cpp
        if (url.find("https://")) {
            port = "443";
            tmp = url.substr(8, url.length());
        } else if (url.find("http://")) {
            port = "80";
            tmp = url.substr(7, url.length());
        } else
            throw std::runtime_error("Wrong url");
        auto const host = tmp.substr(0, url.find('/'));
        auto const target = tmp.substr(url.find('/'), url.length());

        // The io_context is required for all I/O
        boost::asio::io_context ioc;

        // The SSL context is required, and holds certificates
        ssl::context ctx{ ssl::context::sslv23_client };

        // This holds the root certificate used for verification
        load_root_certificates(ctx);

        // Verify the remote server's certificate
        //ctx.set_verify_mode(ssl::verify_peer);

        // These objects perform our I/O
        tcp::resolver resolver{ ioc };
        ssl::stream<tcp::socket> stream{ ioc, ctx };

        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
        {
            boost::system::error_code ec{ static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category() };
            throw boost::system::system_error{ ec };
        }

        // Look up the domain name
        auto const results = resolver.resolve(host, port);

        // Make the connection on the IP address we get from a lookup
        boost::asio::connect(stream.next_layer(), results.begin(), results.end());

        // Perform the SSL handshake
        stream.handshake(ssl::stream_base::client);

        // Set up an HTTP GET request message
        http::request<http::string_body> req{ http::verb::get, target, version };
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Send the HTTP request to the remote host
        http::write(stream, req);

        // This buffer is used for reading and must be persisted
        boost::beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;

        // Receive the HTTP response, html-page in "res"
        http::read(stream, buffer, res);

        // запись html-страницы в поток вывода
        ss << res;

        // Gracefully close the stream
        boost::system::error_code ec;
        stream.shutdown(ec);
        if (ec == boost::asio::error::eof)
        {
            // Rationale:
            // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
            ec.assign(0, ec.category());
        }
        if (ec)
            throw boost::system::system_error{ ec };

        // If we get here then the connection is closed gracefully
    }
    catch (std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return ss;
}







static void producer_fun(const std::string htmlSource, const int depth) {
    if (depth == 0)
        return;

    GumboOutput* parsedBody = gumbo_parse(htmlSource.c_str());

    std::queue<GumboNode*> parseQueue;
    parseQueue.push(parsedBody->root);

    while (!parseQueue.empty())
    {
        GumboNode* node = parseQueue.front();
        parseQueue.pop();

        if (GUMBO_NODE_ELEMENT == node->type)
        {
            GumboAttribute* href = nullptr;
            GumboAttribute* src = nullptr;
            if (node->v.element.tag == GUMBO_TAG_A && (href =
                    gumbo_get_attribute(&node->v.element.attributes, "href"))) {
                //vLinks.push_back(href->value); пихнуть ссылку в стек
            } else
                if (node->v.element.tag == GUMBO_TAG_IMG && (src =
                     gumbo_get_attribute(&node->v.element.attributes, "src"))) {
                    //std::cout << src->value << std::endl; записать ссыль на картинку
                }

            GumboVector* children = &node->v.element.children;
            for (unsigned int i = 0; i < children->length; ++i)
            {
                parseQueue.push(static_cast<GumboNode*>(children->data[i]));
            }
        }
    }

    gumbo_destroy_output(&kGumboDefaultOptions, parsedBody);
    /*
    if (node->type != GUMBO_NODE_ELEMENT || depth == 0) {
        return;
    }
    GumboAttribute* href;
    GumboAttribute* src;
    if (node->v.element.tag == GUMBO_TAG_A &&
          (href = gumbo_get_attribute(&node->v.element.attributes, "href"))) {
        std::cout << href->value << std::endl;
    } else
        if (node->v.element.tag == GUMBO_TAG_IMG &&
              (src = gumbo_get_attribute(&node->v.element.attributes, "src"))) {
            std::cout << src->value << std::endl;
    }

    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
        producer_fun(static_cast<GumboNode*>(children->data[i]), depth - 1);
    }*/
}
