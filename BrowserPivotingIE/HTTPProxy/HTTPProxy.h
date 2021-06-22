#pragma once
#include "customhttparser/message_parser.h"

#include <winsock2.h>

#include <windows.h>

#include <string>


#define DEFAULT_PROXY_PORT "8080"

#define USERAGENT_IE11_W10 "Mozilla/5.0 (Windows NT 10.0; Trident/7.0; rv:11.0) like Gecko"

std::string ProxyHTTPRequest(HTTP::Message& request);
void HandleCONNECTRequest(SOCKET ClientSocket, HTTP::Message& request);

unsigned int SetupProxyServer(PCSTR port);
void CleanupProxyServer();
void NewClientHandler(void* socket);
void run(PCSTR port);
