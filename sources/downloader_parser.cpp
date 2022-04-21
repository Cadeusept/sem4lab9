// Copyright 2021 Your Name <your_email>

#include <downloader_parser.hpp>
#include <queue>
#include <stdexcept>

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

// Performs an HTTP GET and prints the response
[[maybe_unused]] void downloader_fun(LinkStruct input,
                         std::queue<BodyStruct>& vBody,
                         const std::shared_ptr<std::timed_mutex>& body_v_mutex,
                         const std::shared_ptr<std::timed_mutex>& console_mutex,
                         size_t& in_process)
{
    console_mutex->lock();
    std::cout << "Downloader started to work, links in process: "
              << in_process << ". url: " << input.get_url() << std::endl;
    console_mutex->unlock();

    std::string url = input.get_url();
    int depth = input.get_depth();
    bool flag = false;

    std::string output;
    std::string port;
    try
    {
        if (url.find("https://")) {
            port = "443";
            url = url.substr(9, url.length());
            flag = true;
        } else if (url.find("http://")) {
            port = "80";
            url = url.substr(8, url.length());
            flag = false;
        } else {
            console_mutex->lock();
            std::cerr << "Error: " << "Wrong url" << std::endl;
            std::cout << "Downloader finished working, links in process: "
                  << in_process - 1 << std::endl;
            in_process--;
            console_mutex->unlock();
        return;
        }

        std::string host;
        std::string target;
        size_t protocol_len = 7;

        if (url.find('/') != std::string::npos) {
            host = url.substr(0, url.find('/'));
            target = url.substr(url.find('/'),
                                url.length() - protocol_len - url.find('/'));
        } else {
            host = url;
            target = "/";
        }

        if (flag)
            output = download_https(host, port, target);
        else
            output = download_http(host, port, target);
    } catch (std::exception const& e) {
        console_mutex->lock();
        std::cerr << "Error: " << e.what() << std::endl;
        std::cout << "Downloader finished working, links in process: "
                  << in_process - 1 << std::endl;
        in_process--;
        console_mutex->unlock();
        return;
    }

    // запись html-страницы в поток вывода
    body_v_mutex->lock();
    vBody.push(BodyStruct(output, depth));
    body_v_mutex->unlock();

    console_mutex->lock();
    std::cout << "Downloader finished working, links in process: "
              << in_process << std::endl;
    console_mutex->unlock();
}


[[maybe_unused]] void parser_fun(BodyStruct input,
                         std::queue<LinkStruct>& vLinks,
                         const std::shared_ptr<std::timed_mutex>& link_v_mutex,
                         const std::shared_ptr<std::timed_mutex>& file_mutex,
                         const std::shared_ptr<std::timed_mutex>& console_mutex,
                         size_t& in_process,
                         std::ofstream& fout)
{
    if (in_process == 0)
        return;

    console_mutex->lock();
    std::cout << "Parser started to work, links in process: "
              << in_process << std::endl;
    console_mutex->unlock();

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
            } else {
                if (node->v.element.tag == GUMBO_TAG_IMG &&
                        (src = gumbo_get_attribute(&node->v.element.attributes,
                                             "src"))) {
                    file_mutex->lock();
                    fout << src->value << std::endl;  // записать в файл
                    file_mutex->unlock();
                }
            }
            GumboVector* children = &node->v.element.children;
            for (unsigned int i = 0; i < children->length; ++i)
            {
                parseQueue.push(static_cast<GumboNode*>(children->data[i]));
            }
        }
    }

    gumbo_destroy_output(&kGumboDefaultOptions, parsedBody);

    if (in_process == 0)
        return;

    console_mutex->lock();
    std::cout << "Parser finished working, links in process: "
              << in_process - 1 << std::endl;
    in_process--;
    console_mutex->unlock();
}

std::string download_http(std::string host,
                          std::string port,
                          const std::string& target) {
  boost::asio::io_context context;
  tcp::resolver resolver{context};
  tcp::socket socket{context};

  auto const result = resolver.resolve(host, port);

  boost::asio::connect(socket, result.begin(), result.end());

  http::request<http::string_body> req{http::verb::get, target, 10};
  req.set(http::field::host, host);
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

  http::write(socket, req);

  boost::beast::flat_buffer buffer;
  http::response<http::dynamic_body> res;
  http::read(socket, buffer, res);

  // Gracefully close the socket
  boost::system::error_code ec;
  socket.shutdown(tcp::socket::shutdown_both, ec);
  return boost::beast::buffers_to_string(res.body().data());
}

std::string download_https(std::string host,
                           std::string port,
                           const std::string& target) {
  // The io_context is required for all I/O
  boost::asio::io_context context;

  // The SSL context is required, and holds certificates
  ssl::context ssl_context{ssl::context::sslv23_client};

  // This holds the root certificate used for verification
  boost::system::error_code ssl_ec;
  load_root_certificates(ssl_context, ssl_ec);
  if (ssl_ec) {
    throw(std::runtime_error(ssl_ec.message()));
  }

  // These objects perform our I/O
  tcp::resolver resolver{context};
  ssl::stream<tcp::socket> stream{context, ssl_context};

  // Look up the domain name
  auto const results = resolver.resolve(host, port);

  // Make the connection on the IP address we get from a lookup
  boost::asio::connect(stream.next_layer(), results.begin(), results.end());

  // Perform the SSL handshake
  stream.handshake(ssl::stream_base::client);

  // Set up an HTTP GET request message
  http::request<http::string_body> req{http::verb::get, target, 10};
  req.set(http::field::host, host);
  req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

  // Send the HTTP request to the remote host
  http::write(stream, req);

  // This buffer is used for reading and must be persisted
  boost::beast::flat_buffer buffer;

  // Declare a container to hold the response
  http::response<http::dynamic_body> res;

  // Receive the HTTP response
  http::read(stream, buffer, res);

  // Gracefully close the stream
  boost::system::error_code ec;
  stream.shutdown(ec);

  return boost::beast::buffers_to_string(res.body().data());
}


