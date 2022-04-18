// Copyright 2021 Your Name <your_email>

#include <consumer_producer.hpp>
#include <queue>
#include <stdexcept>

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

// Performs an HTTP GET and prints the response
[[maybe_unused]] void downloader_fun(LinkStruct input,
                         std::queue<BodyStruct>& vBody,
                         const std::shared_ptr<std::timed_mutex>& body_v_mutex)
{
    std::cout << "Downloader started to work" << std::endl;

    std::string url = input.get_url();
    int depth = input.get_depth();

    std::string output;
    try
    {
        //auto port = "80";
        //const int version = 11; //http version

        if (url.find("https://")) {
            //port = "443";
            url = url.substr(9, url.length());
        } else if (url.find("http://")) {
            //port = "80";
            url = url.substr(8, url.length());
        } else
            throw std::runtime_error("Wrong url");
        auto const host = url.substr(0, url.find('/'));
        auto const target = url.substr(url.find('/'), url.length());

        //std::cout << host << " " << port << " " << target << " " << version << std::endl;

        //port = "443";
        //version = 11;
        boost::asio::io_context ioc;
        ssl::context ctx{ ssl::context::sslv23_client };
        load_root_certificates(ctx);
        tcp::resolver resolver{ ioc };
        ssl::stream<tcp::socket> stream{ ioc, ctx };
        if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
        {
            boost::system::error_code ec{ static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category() };
            throw boost::system::system_error{ ec };
        }

        auto const results = resolver.resolve(host, "443");
        boost::asio::connect(stream.next_layer(), results.begin(), results.end());
        stream.handshake(ssl::stream_base::client);

        http::request<http::string_body> req{ http::verb::get, target, 11}; //11
        req.set(http::field::host, "443");
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        http::write(stream, req);
        boost::beast::flat_buffer buffer;
        http::response<boost::beast::http::string_body> res;

        http::read(stream, buffer, res);


        boost::system::error_code ec;
        stream.shutdown(ec);

        output = res.body();
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
    vBody.push(BodyStruct(output, depth)); // запись html-страницы в поток вывода
    body_v_mutex->unlock();
    std::cout << "Downloader finished working" << std::endl;
}







[[maybe_unused]] void parser_fun(BodyStruct input,
                         std::queue<LinkStruct>& vLinks,
                         const std::shared_ptr<std::timed_mutex>& link_v_mutex,
                         const std::shared_ptr<std::timed_mutex>& file_mutex,
                         size_t* in_progress,
                         std::ofstream& fout) {
    std::cout << "Parser started to work" << std::endl;

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
    (*in_progress)--;
    std::cout << "Parser finished working" << std::endl;
}
