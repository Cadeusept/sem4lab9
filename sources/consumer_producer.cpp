// Copyright 2021 Your Name <your_email>

#include <consumer_producer.hpp>
#include <queue>
#include <stdexcept>

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

// Performs an HTTP GET and prints the response
[[maybe_unused]] void downloader_fun(std::queue<LinkStruct>& vLinks,
                         std::queue<BodyStruct>& vBody,
                         const std::shared_ptr<std::timed_mutex>&
                             link_v_mutex,
                         const std::shared_ptr<std::timed_mutex>&
                             body_v_mutex,
                         short& downloaded_num)
{
    std::cout << "Downloader started to work" << std::endl;
    downloaded_num++;

    link_v_mutex->lock();
    LinkStruct input = vLinks.front();
    vLinks.pop();
    link_v_mutex->unlock();

    std::string url = input.get_url();
    int depth = input.get_depth();

    std::stringstream ss;
    try
    {
        auto port = "80";
        const int version = 11; //http version

        if (url.find("https://")) {
            port = "443";
            url = url.substr(8, url.length());
        } else if (url.find("http://")) {
            //port = "80";
            url = url.substr(7, url.length());
        } else
            throw std::runtime_error("Wrong url");
        auto const host = url.substr(0, url.find('/'));
        auto const target = url.substr(url.find('/'), url.length());

        // The io_context is required for all I/O
        boost::asio::io_context ioc;

        // The SSL context is required, and holds certificates
        ssl::context ctx{ ssl::context::sslv23_client };

        // This holds the root certificate used for verification
        load_root_certificates(ctx);

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

    body_v_mutex->lock();
    vBody.push(BodyStruct(ss.str(), depth));
    body_v_mutex->unlock();
    std::cout << "Downloader finished working" << std::endl;
}







[[maybe_unused]] void parser_fun(std::queue<LinkStruct>& vLinks,
                         std::queue<BodyStruct>& vBody,
                         const std::shared_ptr<std::timed_mutex>&
                             link_v_mutex,
                         const std::shared_ptr<std::timed_mutex>&
                             body_v_mutex,
                         const std::shared_ptr<std::timed_mutex>& file_mutex,
                         short& parsed_num,
                         std::ofstream& fout) {
    std::cout << "Parser started to work" << std::endl;

    body_v_mutex->lock();
    BodyStruct input = vBody.front();
    vBody.pop();
    body_v_mutex->unlock();

    std::string htmlSource = input.get_body();
    int depth = input.get_depth();

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
            if (depth > 1 && node->v.element.tag == GUMBO_TAG_A && (href =
                    gumbo_get_attribute(&node->v.element.attributes, "href"))) {

                link_v_mutex->lock();
                vLinks.push(LinkStruct(
                    href->value, depth - 1));  // пихнуть ссылку в стек
                link_v_mutex->unlock();
            } else
                if (node->v.element.tag == GUMBO_TAG_IMG && (src =
                     gumbo_get_attribute(&node->v.element.attributes, "src"))) {

                    file_mutex->lock();
                    fout << src->value << std::endl; //записать в файл
                    file_mutex->unlock();
                }

            GumboVector* children = &node->v.element.children;
            for (unsigned int i = 0; i < children->length; ++i)
            {
                parseQueue.push(static_cast<GumboNode*>(children->data[i]));
            }
        }
    }

    gumbo_destroy_output(&kGumboDefaultOptions, parsedBody);
    parsed_num++;
    std::cout << "Parser finished working" << std::endl;
}
