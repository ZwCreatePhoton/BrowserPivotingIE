#include <customhttparser/http.h>
#include <customhttparser/message.h>

namespace HTTP
{
    State Message::state()
    {
        return _state;
    }

    void Message::set_state(State state)
    {
        _state = state;
    }

    bool Message::is_request()
    {
        return _is_request;
    }

    bool Message::is_response()
    {
        return !is_request();
    }

    StartLine &Message::start_line()
    {
        return *_start_line;
    }

    void Message::add_start_line(std::unique_ptr<StartLine> startLine)
    {
//        _is_request =  typeid(*_start_line) == typeid(RequestLine);
        _start_line = std::move(startLine);
    }

    void Message::add_header(std::unique_ptr<Header> header)
    {
        _headers.push_back(std::move(header));
    }

    void Message::add_header(const std::string& name, const std::string& value)
    {
        switch (ParseHeaderName(name))
        {
            case HeaderName::TRANSFER_ENCODING:
                _headers.push_back(std::unique_ptr<Header>(std::make_unique<TransferEncodingHeader>(name, value).release()));
                break;
            case HeaderName::CONTENT_LENGTH:
                _headers.push_back(std::unique_ptr<Header>(std::make_unique<ContentLengthHeader>(name, value).release()));
                break;
            case HeaderName::CONTENT_ENCODING:
            default:
                _headers.push_back(std::make_unique<Header>(name, value));
                break;
        }
    }

    std::vector<std::reference_wrapper<Header>> Message::headers()
    {
        std::vector<std::reference_wrapper<Header>> hs{};
        for (auto &hp : _headers)
            hs.emplace_back(*hp);
        return hs;
    }

    void Message::append_body_data(const std::string& body_data)
    {
        _body += body_data;
    }

    std::string& Message::body()
    {
        return _body;
    }

    std::string Message::normalized_body(bool chunking)
    {
        std::string normalized{};

        // dechunk
        bool should_dechunk = false;
        for (auto &header : headers())
        {
            if (header.get().headername() == HeaderName::TRANSFER_ENCODING)
            {
                if (((TransferEncodingHeader &)header.get()).encoding() == TransferEncodingValue::CHUNKED)
                {
                    should_dechunk = true;
                    break;
                }
            }
        }

        if (chunking && should_dechunk)
        {
            for (auto &chunk : chunks())
                normalized += chunk.data();
        }
        else
            normalized = body();

        //TODO: decompression

        return normalized;
    }

    std::vector<Chunk> &Message::chunks()
    {
        return _chunks;
    }
}