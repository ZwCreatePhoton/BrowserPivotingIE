#include <string>
#include <vector>

#pragma once

namespace HTTP
{
    enum class State
    {
            UNKNOWN = 0,
//        REQUEST_LINE_PARSED,
//        RESPONSE_LINE_PARSED,
            START_LINE_PARSED = 1,
            HEADERS_PARSED = 2,
            BODY_PARSED = 3
    };

    enum class Version
    {
            HTTP_0_9 = 0,
            HTTP_1_0,
            HTTP_1_1
    };

    enum class MethodType
    {
            UNKNOWN = 0,
            OPTIONS,
            GET,
            HEAD,
            POST,
            PUT,
            DELETE,
            TRACE,
            CONNECT,
            PROPFIND,
    };

    enum class HeaderName
    {
            UNKNOWN = 0,
            TRANSFER_ENCODING,
            CONTENT_LENGTH,
            CONTENT_ENCODING
    };

    enum class TransferEncodingValue
    {
            IDENTITY = 0,
            CHUNKED
    };

    MethodType ParseMethod(std::string method);
    Version ParseVersion(const std::string& version);
    uint16_t ParseStatusCode(const std::string& status_code);
    HeaderName ParseHeaderName(std::string name);
    TransferEncodingValue ParseTransferEncodingValue(std::string value);

    class Method
    {
        public:
            explicit Method(const std::string& m);

        public:
            void method(std::string m);
            MethodType type();
            std::string serialize();

        private:
            std::string _method;
            MethodType _methodtype;
    };

    class StartLine {};

    class RequestLine : public StartLine
    {
        public:
            RequestLine(const std::string& method, std::string uri, const std::string& version);
            RequestLine(Method method, std::string uri, Version version);
            RequestLine(const RequestLine &requestLine);

            Method method() {return _method;};
            std::string uri() {return _uri;};
            Version version() {return _version;};

        private:
            Method _method;
            Version _version{};
            std::string _uri{};
    };

    class StatusLine : public StartLine
    {
        public:
            StatusLine(const std::string& version, const std::string& status_code, std::string status_message);
            StatusLine(Version version, uint16_t status_code, std::string status_message);
            StatusLine(const StatusLine &statusLine);

            uint16_t status_code() {return _status_code;};
            std::string status_message() {return _status_message;};
            Version version() {return _version;};

        private:
            uint16_t _status_code{};
            Version _version{};
            std::string _status_message{};
    };

    class Header
    {
        protected:
            Header(std::string name, std::string value, HeaderName headername);

        public:
            Header(std::string name, std::string value);
            HeaderName headername() {return _headername;};
            std::string name() {return _name;};
            std::string value() {return _value;};

        private:
            HeaderName _headername{};
            std::string _name{};
            std::string _value{};
    };

    class TransferEncodingHeader : public Header
    {
        public:
            TransferEncodingHeader(std::string name, const std::string& value);

            TransferEncodingValue encoding() { return _encoding;};

        private:
            TransferEncodingValue _encoding;
    };

    class ContentLengthHeader : public Header
    {
        public:
            ContentLengthHeader(std::string name, const std::string &value);
            uint64_t length() {return _length;};

            static uint64_t parse_length(const std::string &value);

        private:
            uint64_t _length;
    };

    class Chunk
    {
        public:
            Chunk(const Chunk &chunk);
            Chunk(uint64_t size, std::string data);

            uint64_t size() {return _size;};
            std::string data() {return _data;};

            // Parses chunks and returns them.
            // Stop when the zero chunk is encountered or error in parsing
            // TODO: Move parse_chunks out of this class. Maybe to MessageParser or HttpApplication?
            // why? because parse_chunks must use a dechunk algo. and this class is not where we should be using polymorphism
            static std::vector<Chunk> parse_chunks(std::string &data);


        private:
            uint64_t _size{};
            std::string _data{};
    };
}
