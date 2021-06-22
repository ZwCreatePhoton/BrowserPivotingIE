#include <memory>
#include <regex>
#include <string>
#include <utility>
#include <iostream>

#include <customhttparser/message_parser.h>
#include <customhttparser/http.h>


namespace HTTP
{
    std::string METHOD_OPTIONS = "OPTIONS";
    std::string METHOD_GET = "GET";
    std::string METHOD_HEAD = "HEAD";
    std::string METHOD_POST = "POST";
    std::string METHOD_PUT = "PUT";
    std::string METHOD_DELETE = "DELETE";
    std::string METHOD_TRACE = "TRACE";
    std::string METHOD_CONNECT = "CONNECT";
    std::string METHOD_PROPFIND = "PROPFIND";

    std::string HEADER_TRANSFER_ENCODING = "TRANSFER-ENCODING";
    std::string HEADER_CONTENT_LENGTH = "CONTENT-LENGTH";
    std::string HEADER_CONTENT_ENCODING = "CONTENT-ENCODING";

    std::string TRANSERENCODING_IDENTITY = "IDENTITY";
    std::string TRANSERENCODING_CHUNKED = "CHUNKED";

    MethodType ParseMethod(std::string method)
    {
        for (auto &c : method)
            c = toupper(c);

        if (method == METHOD_OPTIONS) return MethodType::OPTIONS;
        else if (method == METHOD_GET) return MethodType::GET;
        else if (method == METHOD_HEAD) return MethodType::HEAD;
        else if (method == METHOD_POST) return MethodType::POST;
        else if (method == METHOD_PUT) return MethodType::PUT;
        else if (method == METHOD_DELETE) return MethodType::DELETE;
        else if (method == METHOD_TRACE) return MethodType::TRACE;
        else if (method == METHOD_CONNECT) return MethodType::CONNECT;
        else if (method == METHOD_PROPFIND) return MethodType::PROPFIND;

        return MethodType::UNKNOWN;
    }

    Method::Method(const std::string& m)
            : _method(m), _methodtype(ParseMethod(m))
    {}

    void Method::method(std::string m)
    {
        _method = std::move(m);
    }

    MethodType Method::type()
    {
        return ParseMethod(_method);
    }

    std::string Method::serialize()
    {
        return _method;
    }

    Version ParseVersion(const std::string& version)
    {
        if (version == "1.0") return Version::HTTP_1_0;
        else if (version == "1.1") return Version::HTTP_1_1;

        return Version::HTTP_0_9;
    }

    uint16_t ParseStatusCode(const std::string& status_code)
    {
        return std::stoi(status_code);
    }

    HeaderName ParseHeaderName(std::string name)
    {
        for (auto &c : name)
            c = toupper(c);
        if (name == HEADER_TRANSFER_ENCODING) return HeaderName::TRANSFER_ENCODING;
        else if (name == HEADER_CONTENT_LENGTH) return HeaderName::CONTENT_LENGTH;
        else if (name == HEADER_CONTENT_ENCODING) return HeaderName::CONTENT_ENCODING;

        return HeaderName::UNKNOWN;
    }

    TransferEncodingValue ParseTransferEncodingValue(std::string value)
    {
        for (auto &c : value)
            c = toupper(c);
        if (value == TRANSERENCODING_CHUNKED) return TransferEncodingValue::CHUNKED;

        return TransferEncodingValue::IDENTITY;
    }

    RequestLine::RequestLine(const std::string& method, std::string uri, const std::string& version) : RequestLine(Method(method), std::move(uri), ParseVersion(version)) {}

    RequestLine::RequestLine(Method method, std::string uri, Version version) : _method(std::move(method)), _uri(std::move(uri)), _version(version) {}

    RequestLine::RequestLine(const RequestLine &requestLine) : _method(requestLine._method), _uri(requestLine._uri), _version(requestLine._version) {}

    StatusLine::StatusLine(const std::string& version, const std::string& status_code, std::string status_message) : StatusLine(ParseVersion(version), ParseStatusCode(status_code), std::move(status_message)) {}

    StatusLine::StatusLine(Version version, uint16_t status_code, std::string status_message) : _version(version), _status_code(status_code), _status_message(std::move(status_message)) {}

    StatusLine::StatusLine(const StatusLine &statusLine) : _version(statusLine._version), _status_code(statusLine._status_code), _status_message(statusLine._status_message) {}

    Header::Header(std::string name, std::string value) : Header(std::move(name), std::move(value), HeaderName::UNKNOWN) {}

    Header::Header(std::string name, std::string value, HeaderName headername) : _name(std::move(name)), _value(std::move(value)), _headername(headername) {}

    TransferEncodingHeader::TransferEncodingHeader(std::string name, const std::string& value) : Header(std::move(name), value, HeaderName::TRANSFER_ENCODING), _encoding(ParseTransferEncodingValue(value)) {}

    ContentLengthHeader::ContentLengthHeader(std::string name, const std::string& value) : Header(std::move(name), value, HeaderName::CONTENT_LENGTH), _length(ContentLengthHeader::parse_length(value)) {}

    uint64_t ContentLengthHeader::parse_length(const std::string &value)
    {
        try
        {
            return std::stol(value);
        }
        catch (std::out_of_range &e)
        {
            return std::numeric_limits<uint64_t>::max();
        }
    }

    Chunk::Chunk(uint64_t size, std::string data) : _size(size), _data(std::move(data)) {}

    Chunk::Chunk(const Chunk &chunk) : Chunk(chunk._size, chunk._data) {}

    std::vector<Chunk> Chunk::parse_chunks(std::string &data)
    {
        std::vector<Chunk> chunks{};

        std::smatch match;

        while(!data.empty())
        {
            match = std::smatch();
            if (std::regex_search(data, match, HTTP::HTTP_CHUNK_HEADER, std::regex_constants::match_continuous))
            {
                // we found a chunk header
                std::string chunk_size_str = match.str(1);
//                std::string chunk_extension = match.str(2);
                char *p;
                long chunk_size = strtol(chunk_size_str.c_str(), &p, 16);
                if (*p != 0)
                {
                    //couldn't parse size...
                    std::cout << "[!]\tstrtol failed to parse the string '" << chunk_size_str << "' into an integer" << std::endl;
                    exit(1);
                }
                if (chunk_size == 0)
                {
                    data.erase(0, match.position() + match.length());
                    chunks.emplace_back(chunk_size, "");
                    break;
                }
                if (data.size() < chunk_size) break;

                std::smatch new_match = std::smatch();
                std::string new_data = std::string(data);
                new_data.erase(0, match.position() + match.length());

                std::string chunk_data = std::string(new_data.begin(), new_data.begin() + chunk_size);
                new_data.erase(0, chunk_size);

                if (!std::regex_search(new_data, new_match, HTTP::HTTP_CHUNK_END_REGEX, std::regex_constants::match_continuous)) break; // Missing valid new line at end of chunk
                new_data.erase(0, new_match.position() + new_match.length() );
                data = new_data;

                chunks.emplace_back(chunk_size, chunk_data); // create chunk object and it to our chunk list
            }
            else
            {
                break;
            }
        }

        return chunks;
    }
}