#define protected public

#include <cstdlib>
#include <iostream>
#include <cassert>
using namespace std::literals;

#include <customhttparser/message_parser.h>



void print_message(HTTP::Message &message)
{
    std::cout << "State: ";
    switch (message.state())
    {
        case HTTP::State::UNKNOWN:
            std::cout << "UNKNOWN";
            break;
        case HTTP::State::START_LINE_PARSED:
            std::cout << "START_LINE_PARSED";
            break;
        case HTTP::State::HEADERS_PARSED:
            std::cout << "HEADERS_PARSED";
            break;
        case HTTP::State::BODY_PARSED:
            std::cout << "BODY_PARSED";
            break;
    }
    std::cout << std::endl;
    auto headers = message.headers();
    for (auto &header : headers)
    {
        std::cout << "Header: (";
        switch (header.get().headername())
        {
            case HTTP::HeaderName::UNKNOWN:
                std::cout << "UNKNOWN";
                break;
            case HTTP::HeaderName::TRANSFER_ENCODING:
                std::cout << "TRANSFER_ENCODING";
                break;
            case HTTP::HeaderName::CONTENT_LENGTH:
                std::cout << "CONTENT_LENGTH";
                break;
            case HTTP::HeaderName::CONTENT_ENCODING:
                std::cout << "CONTENT_ENCODING";
                break;
        }
        std::cout << ", "<< header.get().name() << ", " << header.get().value() << ")" << std::endl;
    }
    std::cout << "Raw body:" << std::endl << message.body() << std::endl;
//    std::cout << "Normalized body: " <<  << std::endl;
}

void test_message_parser1()
{
    // full HTTP response (headers + body) sent as 1 segment

    std::string headers_str = "HTTP/1.1 200 OK\r\n"
                              "Date: Wed, 29 Jan 2020 06:53:40 GMT\r\n"
                              "Content-type: text/html\r\n"
                              "Content-Length: 239\r\n"
                              "Last-Modified: Thu, 04 Oct 2018 18:17:56 GMT\r\n"
                              "\r\n";
    std::string body_str =                           "<!DOCTYPE html>\n"
                                                     "<html>\n"
                                                     "<head>\n"
                                                     "<meta http-equiv=\"X-UA-Compatible\" content=\"IE=EmulateIE8\">\n"
                                                     "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\n"
                                                     "<link rel=\"icon\" href=\"data:,\">\n"
                                                     "</head>\n"
                                                     "<body>\n"
                                                     "This does nothing.\n"
                                                     "</body>\n"
                                                     "</html>";
    std::string raw_body_str = body_str;
    std::string message_str = headers_str + raw_body_str;

    auto parser = HTTP::MessageParser();
    std::vector<uint8_t> segment(message_str.begin(), message_str.end());
    parser.process_segment(segment);

    auto messages = parser.messages();
    assert(messages.size() == 1);
    auto &message = messages[0].get();
//    print_message(message);
    assert(message.body() == raw_body_str);
    assert(message.normalized_body() == body_str);
}

void test_message_parser2()
{
    // full HTTP response (headers + body), declared chunked, sent chunked with chunk size 16, sent as 1 segment

    std::string headers_str = "HTTP/1.1 200 OK\r\n"
                              "Transfer-Encoding: chunked\r\n"
                              "\r\n";
    std::string body_str =                           "<!DOCTYPE html>\n"
                                                     "<html>\n"
                                                     "<head>\n"
                                                     "<meta http-equiv=\"X-UA-Compatible\" content=\"IE=EmulateIE8\">\n"
                                                     "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\n"
                                                     "<link rel=\"icon\" href=\"data:,\">\n"
                                                     "</head>\n"
                                                     "<body>\n"
                                                     "This does nothing.\n"
                                                     "</body>\n"
                                                     "</html>";
    std::string raw_body_str = "10\r\n"
                               "<!DOCTYPE html>\n"
                               "\r\n"
                               "10\r\n"
                               "<html>\n"
                               "<head>\n"
                               "<m\r\n"
                               "10\r\n"
                               "eta http-equiv=\"\r\n"
                               "10\r\n"
                               "X-UA-Compatible\"\r\n"
                               "10\r\n"
                               " content=\"IE=Emu\r\n"
                               "10\r\n"
                               "lateIE8\">\n"
                               "<meta \r\n"
                               "10\r\n"
                               "http-equiv=\"Cont\r\n"
                               "10\r\n"
                               "ent-Type\" conten\r\n"
                               "10\r\n"
                               "t=\"text/html; ch\r\n"
                               "10\r\n"
                               "arset=UTF-8\">\n"
                               "<l\r\n"
                               "10\r\n"
                               "ink rel=\"icon\" h\r\n"
                               "10\r\n"
                               "ref=\"data:,\">\n"
                               "</\r\n"
                               "10\r\n"
                               "head>\n"
                               "<body>\n"
                               "Thi\r\n"
                               "10\r\n"
                               "s does nothing.\n"
                               "\r\n"
                               "f\r\n"
                               "</body>\n"
                               "</html>\r\n"
                               "0\r\n"
                               "\r\n";
    std::string message_str = headers_str + raw_body_str;

    auto parser = HTTP::MessageParser();
    std::vector<uint8_t> segment(message_str.begin(), message_str.end());
    parser.process_segment(segment);

    auto messages = parser.messages();
    assert(messages.size() == 1);
    auto &message = messages[0].get();

//    print_message(message);
    assert(message.body() == raw_body_str);
    assert(message.normalized_body() == body_str);
    assert(message.chunks().size() == 16);
}

void test_message_parser_simplehttpserver()
{
    // full HTTP status line sent in 1 segment followed by rest of the HTTP headers sent by python's SimpleHTTPServer

    std::string status_line_str = "HTTP/1.1 200 OK\r\n";

    std::string headers_str = "Server: SimpleHTTP/0.6 Python/2.7.13\r\n"
                              "Date: Fri, 28 Feb 2020 00:58:30 GMT\r\n"
                              "Content-type: text/html; charset=mbcs\r\n"
                              "Content-Length: 8\r\n"
                              "\r\n";

    std::string body = "deadbeaf";

    std::string segment1 = status_line_str;
    std::string segment2 = headers_str + body;
    std::string segment3 = "";

    auto parser = HTTP::MessageParser();
    std::vector<std::vector<uint8_t >> segments{};
    segments.emplace_back(segment1.begin(), segment1.end());
    segments.emplace_back(segment2.begin(), segment2.end());
    segments.emplace_back(segment3.begin(), segment3.end());

    for (auto segment : segments)
    {
        parser.process_segment(segment);
    }

    auto messages = parser.messages();
    assert(messages.size() == 1);
    auto &message = messages[0].get();
//    print_message(message);
    assert(message.body() == body);
    assert(message.normalized_body() == body);
}

void test_message_parser_simplehttpserver_stoi_error()
{
    // SimpleHTTPServer causes a stoi error when status message is more than 1 word

    std::string status_line_str = "HTTP/1.1 200 foo bar\r\n";
    std::string headers_str1 = "Server: SimpleHTTP/0.6 Python/2.7.15rc1\r\n";

    std::string segment1 = status_line_str;
    std::string segment2 = headers_str1;

    auto parser = HTTP::MessageParser();
    std::vector<std::vector<uint8_t >> segments{};
    segments.emplace_back(segment1.begin(), segment1.end());
    segments.emplace_back(segment2.begin(), segment2.end());

    for (auto segment : segments)
        parser.process_segment(segment);

    auto messages = parser.messages();
}

void test_message_parser_propfind()
{
    std::string status_line_str = "PROPFIND /path/file.txt HTTP/1.1\r\n";
    std::string headers_str1 = "Content-Length: 8\r\n"
                               "\r\n";
    std::string body_str = "AAAAAAAA";

    std::string segment1 = status_line_str + headers_str1 + body_str;

    auto parser = HTTP::MessageParser();
    std::vector<std::vector<uint8_t >> segments{};
    segments.emplace_back(segment1.begin(), segment1.end());

    for (auto segment : segments)
        parser.process_segment(segment);

    auto messages = parser.messages();

    assert(messages.size() == 1);
    HTTP::Message &message = messages[0].get();
    HTTP::RequestLine &requestline = (HTTP::RequestLine &)message.start_line();
    assert(message.is_request());
    assert(requestline.method().type() == HTTP::MethodType::PROPFIND);
    assert(requestline.uri() == "/path/file.txt");
    assert(requestline.version() == HTTP::Version::HTTP_1_1);
}

void test_message_negative_status_code()
{
    std::string segment1 = "HTTP/1.0 -100 OK\r\n"
                           "\r\n";

    auto parser = HTTP::MessageParser();
    std::vector<std::vector<uint8_t >> segments{};
    segments.emplace_back(segment1.begin(), segment1.end());

    for (auto segment : segments)
        parser.process_segment(segment);

    auto messages = parser.messages();

    assert(messages.size() == 1);
    HTTP::Message &message = messages[0].get();
    HTTP::StatusLine &statusline = (HTTP::StatusLine &)message.start_line();
    assert(message.is_response());
    assert(statusline.version() == HTTP::Version::HTTP_1_0);
    assert(statusline.status_code() == (uint16_t)-100);
}

void test_message_large_content_length()
{
    std::string segment1 = "HTTP/1.0 200 OK\r\n"
                           "Content-Length: 18446744073709551615"
                           "\r\n"
                           "AAAAAAAA";

    auto parser = HTTP::MessageParser();
    std::vector<std::vector<uint8_t >> segments{};
    segments.emplace_back(segment1.begin(), segment1.end());
    segments.emplace_back();

    for (auto segment : segments)
        parser.process_segment(segment);

    auto messages = parser.messages();

    assert(messages.size() == 1);
    HTTP::Message &message = messages[0].get();
    HTTP::StatusLine &statusline = (HTTP::StatusLine &)message.start_line();
    assert(message.is_response());
    auto headers = message.headers();
    assert(headers.size() == 1);
    auto &header = headers[0].get();
    assert(header.headername() == HTTP::HeaderName::CONTENT_LENGTH);
    assert(((HTTP::ContentLengthHeader&)header).value() == "18446744073709551615");
    assert(((HTTP::ContentLengthHeader&)header).length() > 8);
    assert(message.body().size() == 8);
    assert(message.body() == "AAAAAAAA");
}

void test_message_parser_no_uri()
{
    std::string status_line_str = "POST  HTTP/1.1\r\n";
    std::string headers_str1 = "Content-Length: 8\r\n"
                               "\r\n";
    std::string body_str = "AAAAAAAA";

    std::string segment1 = status_line_str + headers_str1 + body_str;

    auto parser = HTTP::MessageParser();
    std::vector<std::vector<uint8_t >> segments{};
    segments.emplace_back(segment1.begin(), segment1.end());

    for (auto segment : segments)
        parser.process_segment(segment);

    auto messages = parser.messages();

    assert(messages.size() == 1);
    HTTP::Message &message = messages[0].get();
    HTTP::RequestLine &requestline = (HTTP::RequestLine &)message.start_line();
    assert(message.is_request());
    assert(requestline.method().type() == HTTP::MethodType::POST);
    assert(requestline.uri() == "");
    assert(requestline.version() == HTTP::Version::HTTP_1_1);
}


void test_message_parser_space_in_uri()
{
    std::string status_line_str = "POST /some dir/file.txt HTTP/1.1\r\n";
    std::string headers_str1 = "Content-Length: 8\r\n"
                               "\r\n";
    std::string body_str = "AAAAAAAA";

    std::string segment1 = status_line_str + headers_str1 + body_str;

    auto parser = HTTP::MessageParser();
    std::vector<std::vector<uint8_t >> segments{};
    segments.emplace_back(segment1.begin(), segment1.end());

    for (auto segment : segments)
        parser.process_segment(segment);

    auto messages = parser.messages();

    assert(messages.size() == 1);
    HTTP::Message &message = messages[0].get();
    HTTP::RequestLine &requestline = (HTTP::RequestLine &)message.start_line();
    assert(message.is_request());
    assert(requestline.method().type() == HTTP::MethodType::POST);
    assert(requestline.uri() == "/some dir/file.txt");
    assert(requestline.version() == HTTP::Version::HTTP_1_1);
}

void test_1_chunked_responses_without_nullbytes()
{
    std::string const response1 = "HTTP/1.1 200 OK\r\n"
                                  "Transfer-Encoding: chunked\r\n"
                                  "\r\n"
                                  "14\r\n"
                                  "aaaaaaaaaaaaaaaaaaaa\r\n"s
                                  "0\r\n"
                                  "\r\n";

    std::string segment1 = response1;

    auto parser = HTTP::MessageParser();
    std::vector<std::vector<uint8_t >> segments{};
    segments.emplace_back(segment1.begin(), segment1.end());

    for (auto segment : segments)
        parser.process_segment(segment);

    auto messages = parser.messages();

    assert(messages.size() == 1);
    HTTP::Message &message1 = messages[0].get();
    assert(message1.normalized_body() == "aaaaaaaaaaaaaaaaaaaa"s);
    assert(parser.data.empty());
    assert(parser.unprocessed_chunked_body.empty());
}

void test_1_chunked_responses_with_nullbytes()
{
    std::string const response1 = "HTTP/1.1 200 OK\r\n"
                                  "Transfer-Encoding: chunked\r\n"
                                  "\r\n"
                                  "14\r\n"
                                  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\r\n"s
                                  "0\r\n"
                                  "\r\n";

    std::string segment1 = response1;

    auto parser = HTTP::MessageParser();
    std::vector<std::vector<uint8_t >> segments{};
    segments.emplace_back(segment1.begin(), segment1.end());

    for (auto segment : segments)
        parser.process_segment(segment);

    auto messages = parser.messages();

    assert(messages.size() == 1);
    HTTP::Message &message1 = messages[0].get();
    assert(message1.normalized_body() == "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"s);
    std::cout << parser.data ;
    assert(parser.data.empty());
    assert(parser.unprocessed_chunked_body.empty());
}

void test_2_chunked_responses_without_nullbytes()
{
    std::string const response1 = "HTTP/1.1 200 OK\r\n"
                                  "Transfer-Encoding: chunked\r\n"
                                  "\r\n"
                                  "14\r\n"
                                  "aaaaaaaaaaaaaaaaaaaa\r\n"s
                                  "0\r\n"
                                  "\r\n";

    std::string segment1 = response1;
    std::string segment2 = response1;

    auto parser = HTTP::MessageParser();
    std::vector<std::vector<uint8_t >> segments{};
    segments.emplace_back(segment1.begin(), segment1.end());
    segments.emplace_back(segment2.begin(), segment2.end());

    for (auto segment : segments)
        parser.process_segment(segment);

    auto messages = parser.messages();

    assert(messages.size() == 2);
    HTTP::Message &message1 = messages[0].get();
    assert(message1.normalized_body() == "aaaaaaaaaaaaaaaaaaaa"s);
    HTTP::Message &message2 = messages[1].get();
    assert(message2.normalized_body() == "aaaaaaaaaaaaaaaaaaaa"s);
    assert(parser.data.empty());
    assert(parser.unprocessed_chunked_body.empty());
}

void test_2_chunked_responses_with_nullbytes()
{
    std::string const response1 = "HTTP/1.1 200 OK\r\n"
                            "Transfer-Encoding: chunked\r\n"
                            "\r\n"
                            "14\r\n"
                            "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\r\n"s
                            "0\r\n"
                            "\r\n";

    std::string segment1 = response1;
    std::string segment2 = response1;

    auto parser = HTTP::MessageParser();
    std::vector<std::vector<uint8_t >> segments{};
    segments.emplace_back(segment1.begin(), segment1.end());
    segments.emplace_back(segment2.begin(), segment2.end());

    for (auto segment : segments)
        parser.process_segment(segment);

    auto messages = parser.messages();

    assert(messages.size() == 2);
    HTTP::Message &message1 = messages[0].get();
    assert(message1.normalized_body() == "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"s);
    HTTP::Message &message2 = messages[1].get();
    assert(message2.normalized_body() == "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"s);
    assert(parser.data.empty());
    assert(parser.unprocessed_chunked_body.empty());
}

void test_parser_type1()
{
    std::string const response1 = "HTTP/1.1 200 OK\r\n\r\n";
    std::string segment1 = response1;

    auto parser = HTTP::MessageParser(HTTP::MessageParser::Response);
    std::vector<std::vector<uint8_t >> segments{};
    segments.emplace_back(segment1.begin(), segment1.end());

    for (auto segment : segments)
        parser.process_segment(segment);

    auto messages = parser.messages();

    assert(messages.size() == 1);
    HTTP::Message &message1 = messages[0].get();
    assert(message1.is_response());
}

void test_parser_type2()
{
    std::string const request1 = "POST / HTTP/1.1\r\n\r\n";
    std::string segment1 = request1;

    auto parser = HTTP::MessageParser(HTTP::MessageParser::Response);
    std::vector<std::vector<uint8_t >> segments{};
    segments.emplace_back(segment1.begin(), segment1.end());

    for (auto segment : segments)
        parser.process_segment(segment);

    auto messages = parser.messages();
    assert(messages.size() == 0);
}

void test_parser_type3()
{
    std::string const response1 = "HTTP/1.1 200 OK\r\n\r\n";
    std::string segment1 = response1;

    auto parser = HTTP::MessageParser(HTTP::MessageParser::Request);
    std::vector<std::vector<uint8_t >> segments{};
    segments.emplace_back(segment1.begin(), segment1.end());

    for (auto segment : segments)
        parser.process_segment(segment);

    auto messages = parser.messages();

    assert(messages.size() == 0);
}

void test_parser_type4()
{
    std::string const request1 = "POST / HTTP/1.1\r\n\r\n";
    std::string segment1 = request1;

    auto parser = HTTP::MessageParser(HTTP::MessageParser::Request);
    std::vector<std::vector<uint8_t >> segments{};
    segments.emplace_back(segment1.begin(), segment1.end());

    for (auto segment : segments)
        parser.process_segment(segment);

    auto messages = parser.messages();
    assert(messages.size() == 1);
    HTTP::Message &message1 = messages[0].get();
    assert(message1.is_request());
}


void test_parser_type5()
{
    std::string const response1 = "HTTP/1.1 200 OK\r\n\r\n";
    std::string segment1 = response1;

    auto parser = HTTP::MessageParser(HTTP::MessageParser::Both);
    std::vector<std::vector<uint8_t >> segments{};
    segments.emplace_back(segment1.begin(), segment1.end());

    for (auto segment : segments)
        parser.process_segment(segment);

    auto messages = parser.messages();
    assert(messages.size() == 1);
    HTTP::Message &message1 = messages[0].get();
    assert(message1.is_response());
}

void test_parser_type6()
{
    std::string const request1 = "POST / HTTP/1.1\r\n\r\n";
    std::string segment1 = request1;

    auto parser = HTTP::MessageParser(HTTP::MessageParser::Both);
    std::vector<std::vector<uint8_t >> segments{};
    segments.emplace_back(segment1.begin(), segment1.end());

    for (auto segment : segments)
        parser.process_segment(segment);

    auto messages = parser.messages();
    assert(messages.size() == 1);
    HTTP::Message &message1 = messages[0].get();
    assert(message1.is_request());
}

int main(int argc, char** argv)
{
    test_message_parser1();
    test_message_parser2();
    test_message_parser_simplehttpserver();
    test_message_parser_simplehttpserver_stoi_error();
    test_message_parser_propfind();
    test_message_negative_status_code();
    test_message_large_content_length();
    test_message_parser_no_uri();
    test_message_parser_space_in_uri();
    test_1_chunked_responses_without_nullbytes();
    test_1_chunked_responses_with_nullbytes();
    test_2_chunked_responses_without_nullbytes();
    test_2_chunked_responses_with_nullbytes();
    test_parser_type1();
    test_parser_type2();
    test_parser_type3();
    test_parser_type4();
    test_parser_type5();
    test_parser_type6();
    exit(0);
}