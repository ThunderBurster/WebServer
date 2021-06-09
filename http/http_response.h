#pragma once
#include <string>
#include <memory>

#include "http_request.h"
#include "../utils/file.h"


enum class ResponseState {
    SEND, DONE, SEND_ERROR
};

class HttpResponse {
private:
    std::string m_writeBuffer;
    std::unique_ptr<MapFile> m_pMapFile;
    size_t m_pWrite;

    ResponseState m_state;
public:
    HttpResponse();
    ~HttpResponse();
    void writeOnce(int fd);
    void fillResponseBuffer(HttpRequest &request);
    void init();
    ResponseState getState(); 
private:
    void add_status_line(int code);
    void add_headers(const std::string &keepAlive, const std::string &contentType, const std::string &contentLength);
    void dealWithGet(std::string filePath, bool keepAlive);
    void dealWith400();
};