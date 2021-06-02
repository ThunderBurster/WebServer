#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "../utils/processable.h"

// state for link, parse/parse line, response send
enum HttpState {
    READ_AND_PROCESS, WRITE, ERROR, CLOSE
};
enum RequestState {
    REQUESTLINE, HEADER, CONTENT, ERROR, DONE
};
enum ResponseState {
    SEND, DONE, ERROR
};
enum LineState {
    LINE_OPEN, LINE_OK, LINE_BAD
};



class HttpRequest {
private:
    // input and process
    std::string m_readBuffer;
    size_t m_pLine;
    
    RequestState m_state;
    LineState m_lineState;

    // result from parser
    std::string m_method, m_path, m_version, m_body;
    std::unordered_map<std::string, std::string> m_header;
    std::unordered_map<std::string, std::string> m_post;


public:
    HttpRequest();
    ~HttpRequest();
    void readOnce(int fd);
    void tryParse();
    void init();
    RequestState getState();
private:
    void parseRequestLine(const std::string &line);
    void parseHeader(const std::string &line);
    void parseBody(const std::string &line);
    std::string getLine();

};


class HttpResponse {
private:
    std::string m_writeBuffer;
    size_t m_pWrite;

    ResponseState m_state;
public:
    HttpResponse();
    ~HttpResponse();
    void writeOnce(int fd);
    void fillResponseBuffer(HttpRequest &request);
    void init();
    ResponseState getState(); 
};


class HttpConn: public processable {
private:
    
public:

};