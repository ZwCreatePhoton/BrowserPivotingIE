#undef UNICODE
#define WIN32_LEAN_AND_MEAN

#include "HTTPProxy.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <wininet.h>


#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <regex>
#include <algorithm>
#include <assert.h>


#pragma comment (lib, "Wininet.lib")
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "HTTPParser.lib")



#include <regex>


#define DEBUG 1

#define DEFAULT_BUFLEN 2048

#define PROXY_SETUP_SUCCESS 0
#define PROXY_SETUP_ERROR 1

std::string ProxyHTTPRequest(HTTP::Message& request)
{
    std::string host;
    std::string path;
    std::string method;
    std::string body;

    HTTP::RequestLine& requestline = (HTTP::RequestLine&)request.start_line();
    
    // extract the HTTP method
    method = requestline.method().serialize();

    // extract the target HTTP host
    auto headers = request.headers();
    for (auto &header : headers)
    {
        HTTP::Header& rHeader = header.get();
        std::string headername(rHeader.name());

        for (auto& c : headername)
            c = tolower(c);
        if (headername == "host")
        {
            host = rHeader.value();
        }
    }

    // extract the target HTTP relative path
    if (requestline.uri().at(0) == '/')
    {
        path = requestline.uri();
    }
    else
    {
        std::string uri = requestline.uri();
        std::size_t found = uri.find(host);
        if (found != std::string::npos)
        {
            path = std::string(uri.begin() + found + host.length(), uri.end());
        }
        else
        {
#ifdef DEBUG
            printf("Error finding HTTP relative path of the request\n");
#endif
            return std::string{};
        }
    }
    // extract the normalized HTTP body
    body = request.normalized_body();

    HINTERNET hOpenHandle = InternetOpenA(
        USERAGENT_IE11_W10,
        INTERNET_OPEN_TYPE_PRECONFIG,
        NULL,
        NULL,
        0);
    if (hOpenHandle == NULL)
    {
#ifdef DEBUG
        printf("InternetOpenA failed with error: %ld\n", GetLastError());
#endif
        return std::string{};
    }

    HINTERNET hConnectHandle = InternetConnectA(
        hOpenHandle,
        host.data(), // TODO: extract host from hostnames with port number attached
        INTERNET_INVALID_PORT_NUMBER,
        NULL,
        NULL,
        INTERNET_SERVICE_HTTP,
        0,
        0);
    if (hConnectHandle == NULL)
    {
#ifdef DEBUG
        printf("InternetConnectA failed with error: %ld\n", GetLastError());
#endif
        InternetCloseHandle(hOpenHandle);
        return std::string{};
    }

    HINTERNET hRequestHandle = HttpOpenRequestA(
        hConnectHandle,
        method.data(),
        path.data(), // TODO: extract port from hostnames with port number attached
        NULL,
        NULL,
        NULL, // TODO: extract accept types
        INTERNET_FLAG_KEEP_CONNECTION,
        0);
    if (hRequestHandle == NULL)
    {
#ifdef DEBUG
        printf("HttpOpenRequestA failed with error: %ld\n", GetLastError());
#endif
        InternetCloseHandle(hConnectHandle);
        InternetCloseHandle(hOpenHandle);
        return std::string{};
    }

    // Forward & filter HTTP headers
    for (auto& header : headers)
    {
        HTTP::Header& rHeader = header.get();
        std::string headername = rHeader.name();

        for (auto& c : headername)
            c = tolower(c);
        if (headername == "host") {} // header will be added by WinINet
        else if (headername == "user-agent") {} // header will be added by WinINet
        else if (headername == "content-length") {} // header will be added by WinINet
        else if (headername == "connection") {} // header will be added by WinINet
        else
        {
            std::string headerString = rHeader.name() + ": " + rHeader.value() + "\r\n";
            HttpAddRequestHeadersA(
                hRequestHandle,
                headerString.data(),
                headerString.length(),
                HTTP_ADDREQ_FLAG_ADD
            );
        }
    }

    BOOL sent = HttpSendRequestA(
        hRequestHandle,
        NULL,
        0,
        body.data(), // Forward body
        body.length());

    if (!sent)
    {
        // TODO: Send appropriate HTTP error back to client if target HTTP Server is down.

#ifdef DEBUG
        printf("HttpSendRequestA failed with error: %ld\n", GetLastError());
#endif
        InternetCloseHandle(hRequestHandle);
        InternetCloseHandle(hConnectHandle);
        InternetCloseHandle(hOpenHandle);
        return std::string{};
    }

    // read response
    std::string response;
    // response headers
    char responseHeaders[1024];
    DWORD responseHeadersSize = sizeof(responseHeaders);
    if (!HttpQueryInfo(hRequestHandle,
        HTTP_QUERY_RAW_HEADERS_CRLF,
        &responseHeaders,
        &responseHeadersSize,
        NULL))
    {
#ifdef DEBUG
        printf("HttpQueryInfo failed with error: %ld\n", GetLastError());
#endif
        InternetCloseHandle(hRequestHandle);
        InternetCloseHandle(hConnectHandle);
        InternetCloseHandle(hOpenHandle);
        return std::string{};
    }
    response = std::string(responseHeaders);
    // response body
    DWORD blocksize = 4096;
    DWORD received = 0;
    std::string block(blocksize, 0);
    while (InternetReadFile(hRequestHandle, &block[0], blocksize, &received) && received)
    {
        block.resize(received);
        response += block;
    }
    
    // cleanup
    InternetCloseHandle(hRequestHandle);
    InternetCloseHandle(hConnectHandle);
    InternetCloseHandle(hOpenHandle);
    
    return response;
}


#define HTTP_CONNECT_SUCCESS "HTTP/1.1 200 OK\r\n\r\n"
#define HTTP_CONNECT_ERROR "HTTP/1.1 599 Network connect timeout error\r\n\r\n"

void HandleCONNECTRequest(SOCKET ClientSocket, HTTP::Message& request)
{
    // Open TCP connection with target host 
    HTTP::RequestLine& requestline = (HTTP::RequestLine&)request.start_line();
    std::string host_port = requestline.uri();
    std::string host;
    INTERNET_PORT port;

    // This host:port splitting will only work for IPv4
    size_t colon_pos = host_port.find(':');
    if (colon_pos == std::string::npos)
    {
        host = host_port;
        port = INTERNET_INVALID_PORT_NUMBER;
    }
    else
    {
        host = host_port.substr(0, colon_pos);
        port = stoi(host_port.substr(colon_pos + 1));
    }

    bool connect_success;
    HINTERNET hOpenHandle = InternetOpenA(
        USERAGENT_IE11_W10,
        INTERNET_OPEN_TYPE_PRECONFIG,
        NULL,
        NULL,
        0);
    if (hOpenHandle == NULL)
    {
#ifdef DEBUG
        printf("InternetOpenA failed with error: %ld\n", GetLastError());
#endif
        connect_success = false;
    }
    else
    {
        HINTERNET hConnectHandle = InternetConnectA(
            hOpenHandle,
            host.data(),
            port,
            NULL,
            NULL,
            INTERNET_SERVICE_HTTP,
            0,
            0);
        if (hConnectHandle == NULL)
        {
#ifdef DEBUG
            printf("InternetConnectA failed with error: %ld\n", GetLastError());
#endif
            InternetCloseHandle(hOpenHandle);
            connect_success = false;
        }
        else
        {
            connect_success = true;
        }
    }

    // Send TCP connection success/error back to client 
    std::string response = connect_success ? HTTP_CONNECT_SUCCESS : HTTP_CONNECT_ERROR;
    int iSendResult = send(ClientSocket, response.data(), response.length(), 0);
    if (iSendResult == SOCKET_ERROR)
    {
#ifdef DEBUG
        printf("send failed with error: %d\n", WSAGetLastError());
#endif
        return;
    }
    if (!connect_success)
        return;

    // Client should begin to tunnel their HTTP(S) traffic now.
    // TODO: Check if traffic is SSL/TLS traffic (next byte is 0x16)

}

WSADATA wsaData;
SOCKET ListenSocket = INVALID_SOCKET;

unsigned int SetupProxyServer(PCSTR port)
{
    // Initialize Winsock
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
#ifdef DEBUG
        printf("WSAStartup failed with error: %d\n", iResult);
#endif
        return PROXY_SETUP_ERROR;
    }

    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    struct addrinfo* result = NULL;
    iResult = getaddrinfo(NULL, port, &hints, &result);
    if (iResult != 0)
    {
#ifdef DEBUG
        printf("getaddrinfo failed with error: %d\n", iResult);
#endif
        CleanupProxyServer();
        return PROXY_SETUP_ERROR;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET)
    {
#ifdef DEBUG
        printf("socket failed with error: %ld\n", WSAGetLastError());
#endif
        freeaddrinfo(result);
        CleanupProxyServer();
        return PROXY_SETUP_ERROR;
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
#ifdef DEBUG
        printf("bind failed with error: %d\n", WSAGetLastError());
#endif
        freeaddrinfo(result);
        CleanupProxyServer();
        return PROXY_SETUP_ERROR;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
#ifdef DEBUG
        printf("listen failed with error: %d\n", WSAGetLastError());
#endif
        CleanupProxyServer();
        return PROXY_SETUP_ERROR;
    }

    return PROXY_SETUP_SUCCESS;
}

void CleanupProxyServer()
{
    // cleanup
    if (ListenSocket != INVALID_SOCKET)
        closesocket(ListenSocket);
    WSACleanup();
}

void NewClientHandler(void* socket)
{
    int iResult;
    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    SOCKET ClientSocket = (SOCKET)socket;

    uint16_t messageCount = 0;
    auto parser = HTTP::MessageParser();
    do
    {
        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0)
        {
#ifdef DEBUG
            printf("Bytes received: %d\n", iResult);
#endif
            std::vector<uint8_t> segment(recvbuf, recvbuf + iResult);
            parser.process_segment(segment);
        }
        else if (iResult == 0)
        {
#ifdef DEBUG
            printf("Connection closing...\n");
#endif
            std::vector<uint8_t> segment{};
            parser.process_segment(segment);
        }
        else
        {
#ifdef DEBUG
            printf("recv failed with error: %d\n", WSAGetLastError());
#endif
            closesocket(ClientSocket);
            return;
        }

        // check if we have unprocessed complete HTTP messagees
        while (parser.messages().size() > messageCount)
        {
            HTTP::Message& message = parser.messages()[messageCount];
            bool messageParsed = false;
            if (message.state() == HTTP::State::BODY_PARSED)
                messageParsed = true;
            if (message.state() == HTTP::State::HEADERS_PARSED)
            {
                bool containsContentLengthHeader = false;
                bool containsTransferEncodingHeader = false;
                for (auto& header : message.headers())
                {
                    if (header.get().headername() == HTTP::HeaderName::CONTENT_LENGTH)
                    {
                        containsContentLengthHeader = true;
                        auto content_length = ((HTTP::ContentLengthHeader&)header.get()).length();
                        if (content_length == 0) // okay to assume that if CL = 0 then there is no body -> Message parsing complete?
                        {
                            messageParsed = true;
                            break;
                        }
                    }
                    if (header.get().headername() == HTTP::HeaderName::TRANSFER_ENCODING)
                    {
                        containsTransferEncodingHeader = true;
                    }
                }
                if (!containsContentLengthHeader && !containsTransferEncodingHeader)
                {
                    messageParsed = true;
                }
            }

            if (!messageParsed)
                break;

            messageCount += 1;

            if (!message.is_request())
            {
#ifdef DEBUG
                printf("HTTP message is not a request!\n");
#endif
                break;
            }

            HTTP::RequestLine& requestline = (HTTP::RequestLine&)message.start_line();            
            if (requestline.method().serialize() == "CONNECT")
            {
                HandleCONNECTRequest(ClientSocket, message);
                break;
            }
            else
            {
                // Proxy the new message to the target host
                std::string response = ProxyHTTPRequest(message);
                iSendResult = send(ClientSocket, response.data(), response.length(), 0);
                if (iSendResult == SOCKET_ERROR)
                {
#ifdef DEBUG
                    printf("send failed with error: %d\n", WSAGetLastError());
#endif
                    break;
                }
            }
        }

        // Force no support for multiple requests in 1 TCP stream
        if (messageCount == 1)
            break;

    } while (iResult > 0);

    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR)
    {
#ifdef DEBUG
        printf("shutdown failed with error: %d\n", WSAGetLastError());
#endif
        closesocket(ClientSocket);
        return;
    }

    closesocket(ClientSocket);
}

void run(PCSTR port)
{
    if (SetupProxyServer(port) != PROXY_SETUP_SUCCESS)
    {
#ifdef DEBUG
        printf("Proxy setup failed with error: %d\n", WSAGetLastError());
#endif
        CleanupProxyServer();
        return;
    }

    SOCKET ClientSocket;
    while ((ClientSocket = accept(ListenSocket, NULL, NULL)))
    {
        if (ClientSocket == INVALID_SOCKET)
        {
#ifdef DEBUG
            printf("accept failed with error: %d\n", WSAGetLastError());
#endif
            CleanupProxyServer();
            return;
        }
        HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)NewClientHandler, (void*)ClientSocket, 0, NULL);
    }

    CleanupProxyServer();
    return;
}