#pragma once
#include <string>
#include <unordered_map>

enum class RequestState {
    REQUESTLINE, HEADER, CONTENT, RECV_ERROR, PARSE_ERROR, DONE
};
enum class LineState {
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
    void init(bool all);
    RequestState getState();
    std::string getMethod();
    std::string getFilePath();
    bool getkeepAlive();
private:
    void parseRequestLine(const std::string &line);
    void parseHeader(const std::string &line);
    void parseBody(const std::string &line);
    std::string getLine();

};