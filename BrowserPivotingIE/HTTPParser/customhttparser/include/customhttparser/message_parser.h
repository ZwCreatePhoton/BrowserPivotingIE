#include <regex>

#include "http.h"
#include "message.h"


// Custom HTTP parser
// TODO: move to a separate project ?

namespace HTTP
{
    // Modify this so that we accept URI only for pre HTTP1.0 clients (https://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol#h2)
    // TODO: lowercase methods
    const std::regex HTTP_REQUEST_LINE_REGEX(R"(^(OPTIONS|GET|HEAD|POST|PUT|DELETE|TRACE|CONNECT|BCOPY|BDELETE|BMOVE|BPROPFIND|BPROPATH|COPY|DELETE|LOCK|MKCOL|MOVE|NOTIFY|POLL|PROPFIND|PROPPATCH|SEARCH|SUBSCRIBE|UNLOCK|UNSUBSCRIBE|ACL|BASELINE-CONTROL|BIND|CHECKIN|CHECKOUT|LABEL|LINK|MERGE|MKACTIVITY|MKCALENDAR|MKREDIRECTREF|MKWORKSPACE|ORDERPATCH|PATCH|PRI|REBIND|REPORT|UNBIND|UNCHECKOUT|UNLINK|UPDATE|UPDATEREDIRECTREF|VERSION-CONTROL)[^\S\r\n]+([^\r\n]*)[^\S\r\n]+HTTP\/(\S*)[^\S\r\n]*(?:\r\n|\n|\r))"); // captures: method, uri, http version

    const std::regex HTTP_STATUS_LINE_REGEX(R"(^HTTP\/(\S*)[^\S\r\n]*([\w-]*)[^\S\r\n]*(.*)[^\S\r\n]*(?:\r\n|\n|\r))"); // captures: http version, status code, status message

    const std::regex HTTP_HEADER_REGEX(R"(^[^\S\r\n]*(\S+)[^\S\r\n]*(?::)[^\S\r\n]*(.*)[^\S\r\n]*(?:\r\n|\n|\r))"); // captures: header field name, header field value

    const std::regex HTTP_HEADERS_END_REGEX(R"(^(?:\r\n|\n|\r))");

    // https://tools.ietf.org/html/rfc2616#section-3.6.1
    // Tolerant to whitespace before the chunksize
    // Tolerant to whitespace (excludes \n & \r) after the chunksize
    // The term "chunk header" represents what the RFC calls "chunk" minus "chunk-data CRLF". This is equivilant to "chunk-size [ chunk-extension ] CRLF"
    // Possible implementation deviation: final new line: (?:\r\n|\n|\r) or (?:\r\n)
    // (?:\r\n|\n|\r) might bring some problems when chunk-size = 1 & newline token = "\r" & chunk-data = "\n"
    const std::regex HTTP_CHUNK_HEADER(R"(^[^\S]*([A-Fa-f0-9]*)[^\S\r\n]*(;[^\n\r]*)?(?:\r\n))"); // captures: chunk-size, chunk-extension

    const std::regex HTTP_CHUNK_END_REGEX(R"(^(?:\r\n|\n|\r))");

    const std::regex HTTP_CHUNK_EXTENSION(R"(^[^\S]*;[^\S\r\n]*(\S+)[^\S\r\n]*(?:=[^\S\r\n]*([^;\r\n]*))?[^\S\r\n]*)"); // captures: chunk-ext-name, chunk-ext-val (if present)

    // Can we hide data between chunk-data & the next chunk-size ?

    // Will produce a vector of HTTP::Message
    class MessageParser
    {
        public:
            enum ParserType
            {
                Both = 0,
                Request,
                Response,
            };

            MessageParser() = default;
            explicit MessageParser(ParserType parser_type) : type(parser_type) {};
            void process_segment(std::vector<uint8_t> &segment);
            void process_segments(std::vector<std::vector<uint8_t>> &segments);
            std::vector<std::reference_wrapper<Message>> messages();

        protected:
            std::string data{}; // "data" is the data at index "index" of the processed stream (the collection of segments)
            uint64_t index = 0;
            std::string unprocessed_chunked_body{};

            std::vector<std::unique_ptr<Message>> _messages{};

//            std::vector<std::vector<uint8_t>> segments;

        private:
            ParserType type = Both;



    };

}