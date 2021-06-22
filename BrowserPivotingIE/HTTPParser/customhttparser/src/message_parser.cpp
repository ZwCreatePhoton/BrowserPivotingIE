#include <regex>
#include <cassert>
#include <iostream>
#include <cstdlib>
#include <optional>

#include <customhttparser/message_parser.h>


namespace HTTP
{
    void MessageParser::process_segments(std::vector<std::vector<uint8_t>> &segments)
    {
        for (auto &segment : segments)
            process_segment(segment);
    }

    std::vector<std::reference_wrapper<Message>> MessageParser::messages()
    {
        std::vector<std::reference_wrapper<Message>> ms{};
        for (auto &mp : _messages)
            ms.emplace_back(*mp);
        return ms;
    }

    void HTTP::MessageParser::process_segment(std::vector<uint8_t> &segment)
    {
        if (segment.empty()) // connection closed
        {
            if (_messages.empty() && data.empty())
            {
                // Connection closed before there was any data to parse
                ;
            }
            else if (_messages.empty() && !data.empty())
            {
                // Connection closed before we received a complete status-line
                // OR
                // this is a HTTP/0.9 response
                // Either way, all of the data is considered the body
                _messages.push_back(std::make_unique<Message>());
                _messages[0]->append_body_data(data);
                _messages[0]->set_state(State::BODY_PARSED);
            }
            else if (!_messages.empty() && data.empty())
            {
                _messages[_messages.size()-1]->set_state(State::BODY_PARSED);
            }
            else // !message.empty() && !data.empty()
            {
                // Will there every be a sceniero where I'd have to append data to _messages[_messages.size()-1]._body even if that means the body length exceeds the content length ?
                // Maybe a DUT might server up CL thats off by 1 (but still renderable)

                // Connection closed mid HTTP header parse ?
                // or maybe HTTP/0.9 with body?
                // TODO: confirm how to handle this
//                _messages.push_back(std::make_unique<Message>());
                _messages[_messages.size()-1]->append_body_data(data);
                _messages[_messages.size()-1]->set_state(State::BODY_PARSED);
            }
            return;
        }

        std::smatch match = std::smatch(); // do i have to constantally reinit this var to clear it?
        data += std::string(segment.begin(), segment.end());


        // There are 0 messages or all messages previously parsed are complete ; we are ready to parse a new message
        if (_messages.empty() || _messages[_messages.size()-1]->state() == State::BODY_PARSED)
        {
            match = std::smatch();
            if ((type == Both || type == Request) && std::regex_search(data, match, HTTP::HTTP_REQUEST_LINE_REGEX, std::regex_constants::match_continuous))
            {
                // we found a request message
                std::string method = match.str(1);
                std::string uri = match.str(2);
                std::string version = match.str(3);
                _messages.push_back(std::make_unique<Message>());
                _messages[_messages.size()-1]->add_start_line(std::unique_ptr<StartLine>(std::make_unique<RequestLine>(method, uri, version).release()));
                _messages[_messages.size()-1]->set_state(State::START_LINE_PARSED);
                _messages[_messages.size()-1]->_is_request = true;
            }
            else
            {
                match = std::smatch();
                if ((type == Both || type == Response) && std::regex_search(data, match, HTTP::HTTP_STATUS_LINE_REGEX, std::regex_constants::match_continuous))
                {
                    // we found a response message
                    std::string version = match.str(1);
                    std::string status_code = match.str(2);
                    std::string status_message = match.str(3);
                    _messages.push_back(std::make_unique<Message>());
                    _messages[_messages.size()-1]->add_start_line(std::unique_ptr<StartLine>(std::make_unique<StatusLine>(version, status_code, status_message).release()));
                    _messages[_messages.size()-1]->set_state(State::START_LINE_PARSED);
                    _messages[_messages.size()-1]->_is_request = false;
                }
                else
                {
                    // We still have yet to find a message
                    return;
                }
            }
            // We have just finished parsing the status line of our first message
            data.erase(0, match.position() + match.length());
        }

        // There is at least 1 message
        // The parsing of all but the last one are assumed to be complete
        Message &current_message = *_messages[_messages.size()-1];

        // Parse header(s), if any
        while (current_message.state() < State::HEADERS_PARSED)
        {
            match = std::smatch();
            if (std::regex_search(data, match, HTTP::HTTP_HEADERS_END_REGEX, std::regex_constants::match_continuous)) // Empty header -> end of headers reached
            {
                current_message.set_state(State::HEADERS_PARSED);
                data.erase(0, match.position() + match.length());
                break;
            }

            match = std::smatch();
            if (std::regex_search(data, match, HTTP::HTTP_HEADER_REGEX, std::regex_constants::match_continuous)) // (a) header found
            {
                // We found a header, lets add it to our list of headers
                std::string field_name = match.str(1);
                std::string field_value = match.str(2);
                current_message.add_header(field_name, field_value);
//                current_message.add_header(std::make_unique<Header>(field_name, field_value));
                data.erase(0, match.position() + match.length());
            }
            else
            {
                // Partial (or no) header
                // Lets come back later so that we can try again with more segments
                return;
            }
        }

        // HTTP Header for current_message is complete

        // !!! Evasion idea: Mix HTTP versions in an HTTP connection with multiple transactions.

        // Per RFC (https://tools.ietf.org/html/rfc2616#section-4.4):
        // If T-E: Chunked header is present then the transfer-length determined by the chunk algo (or connection close)
        // ElseIf C-L: header is present then the transfer-length is determined by this value
        // Else: connection closing determines transfer-length


        bool chunking = false;
        std::optional<uint64_t> content_length = std::nullopt;
        for (auto &hp : current_message.headers())
        {
            if (hp.get().headername() == HeaderName::TRANSFER_ENCODING)
            {
                if (((TransferEncodingHeader &)hp.get()).encoding() == TransferEncodingValue::CHUNKED)
                {
                    chunking = true;
                    break;
                }
            }
            else if (hp.get().headername() == HeaderName::CONTENT_LENGTH)
            {
                content_length = ((ContentLengthHeader &)hp.get()).length();
            }
        }

        if (chunking)
        {
            // Check If current_message.body() ends with a zero chunk -> body is finished being parsed

            std::string chunks_data_to_parse = unprocessed_chunked_body + data;
            auto chunks = Chunk::parse_chunks(chunks_data_to_parse);
            if (!chunks.empty())
                current_message.chunks().insert(current_message.chunks().end(), chunks.begin(), chunks.end());
            unprocessed_chunked_body = chunks_data_to_parse;

            // Either we have yet to obtain all chunks for this body, (there is no zero chunk)
            // or we have exactly obtained all chunks for this body, (there is at least 1 zero chunk and there are no bytes after it)
            // or we have obtained all chunks for this body plus some bytes leading into the next message (there is at least 1 zero chunk and there are bytes after the first zero chunk)

            if (chunks.empty() || (chunks[chunks.size()-1].size() != 0)) // no zero chunk found
            {
                // We are still waiting on more data
                current_message.append_body_data(data);
                data.erase(0, data.size());
            }
            else// if (chunks[chunks.size()-1].size() == 0) // zero chunk found
            {
                auto dif_size = data.size() - chunks_data_to_parse.size();
                current_message.append_body_data(std::string(data.begin(), data.begin() + dif_size));
                data.erase(0, dif_size);

                // Have we parsed the trailer?
                // Have we parsed the last CRLF?

                //TODO: parser trailer headers

                // Parse CRLF
                match = std::smatch();
                if (std::regex_search(chunks_data_to_parse, match, HTTP::HTTP_CHUNK_END_REGEX, std::regex_constants::match_continuous))
                {
                    // We have successfully parsed the last CRLF
                    // Therefore we have successfully the Chunked-Body
                    current_message.append_body_data(std::string(chunks_data_to_parse.begin(), chunks_data_to_parse.begin() + match.length()));
                    current_message.set_state(State::BODY_PARSED);
                    data.erase(0, match.position() + match.length());
                    unprocessed_chunked_body = data;
                }
            }
        }
        else
        {
            // Check Content-Length
            if (content_length.has_value())
            {
                if (current_message.body().size() + data.size() < content_length.value())
                {
                    current_message.append_body_data(data);
                    data.erase(0, data.size());
                }
                else if(current_message.body().size() + data.size() == content_length.value())
                {
                    current_message.append_body_data(data);
                    current_message.set_state(State::BODY_PARSED);
                    data.erase(0, data.size());
                }
                else // current_message.body().size() + data.size() > content_length.value()
                {
                    auto length_left = content_length.value() - current_message.body().size(); // Amount we need to append to body to complete it
                    current_message.append_body_data(std::string(data.begin(), data.begin() + length_left));
                    current_message.set_state(State::BODY_PARSED);
                    data.erase(0, length_left);
                }
            }
            else
            {
                // wait for connection close
            }
        }
    }
}
