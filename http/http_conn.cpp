#include <sys/socket.h>
#include <cstring>
#include <regex>

#include "http_conn.h"

// HttpRequest
HttpRequest::HttpRequest() {
    m_pLine = 0;
    m_state = RequestState::REQUESTLINE;
    m_lineState = LineState::LINE_OPEN;
}

HttpRequest::~HttpRequest() {
    // empty
}

void HttpRequest::readOnce(int fd) {
    // default to be ET
    static const int READ_BUFFER_SIZE = 2048;
    char buffer[READ_BUFFER_SIZE];
    while(true) {
        memset(buffer, '\0', READ_BUFFER_SIZE);
        ssize_t bytes = recv(fd, buffer, READ_BUFFER_SIZE-1, 0);
        if(bytes == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            }
            else {
                // something wrong, set the state
                m_state = RequestState::ERROR;
                return;
            }
        }
        else if(bytes == 0) {
            m_state = RequestState::ERROR;
            return;
        }
        else {
            // read some bytes
            m_readBuffer.append(buffer);
        }
    }
}

void HttpRequest::tryParse() {
    while(m_state != RequestState::DONE && m_state != RequestState::ERROR) {
        // getLine may change the line state, not the request state
        std::string line = this->getLine();
        if(m_lineState == LineState::LINE_OPEN) {
            break;
        }
        else if(m_lineState == LineState::LINE_BAD) {
            m_state = RequestState::ERROR;
            break;
        }
        else {
            // line ok
        }
        
        
        switch(m_state) {
            case RequestState::REQUESTLINE:
            this->parseRequestLine(line);
            break;

            case RequestState::HEADER:
            this->parseHeader(line);
            break;

            case RequestState::CONTENT:
            this->parseBody(line);
            break;

            case RequestState::ERROR:
            break;

            case RequestState::DONE:
            break;
        }
    }
}

void HttpRequest::init() {
    m_readBuffer.clear();
    m_pLine = 0;
    m_state = RequestState::REQUESTLINE;
    m_lineState = LineState::LINE_OPEN;
    m_method = "", m_path = "", m_version = "", m_body = "";
    m_header.clear();
    m_post.clear();
}

RequestState HttpRequest::getState() {
    return m_state;
}

void HttpRequest::parseRequestLine(const std::string &line) {
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if(std::regex_match(line, subMatch, patten)) {
        m_method = subMatch[1];
        m_path = subMatch[2];
        m_version = subMatch[3];
        m_state = RequestState::HEADER;
    }
    else {
        // failed
        m_state = RequestState::ERROR;        
    }
}

void HttpRequest::parseHeader(const std::string &line) {
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if(std::regex_match(line, subMatch, patten)) {
        m_header[subMatch[1]] = subMatch[2];
    }
    else if(line.empty()) {
        // failed by empty str, empty line
        if(m_method == "POST")
            m_state = RequestState::CONTENT;
        else if(m_method == "GET")
            m_state = RequestState::DONE;
        else
            m_state = RequestState::ERROR;  // not supported method
    }
    else {
        // failed
        m_state = RequestState::ERROR;
    }

}

void HttpRequest::parseBody(const std::string &line) {
    // to do
    // set DONE right now, no parse post(just get right now)
    m_state = RequestState::DONE;
}

std::string HttpRequest::getLine() {
    size_t rpos = m_readBuffer.find("\r", m_pLine);
    size_t npos = m_readBuffer.find("\n", m_pLine);
    std::string line;
    if(rpos != std::string::npos && npos != std::string::npos) {
        // both find
        if(rpos+1 == npos) {
            size_t len = rpos - m_pLine;
            line = m_readBuffer.substr(m_pLine, len);
            m_lineState = LineState::LINE_OK;
            m_pLine = npos + 1;
        }
        else {
            m_lineState = LineState::LINE_BAD;
        }
    }
    else if(rpos != std::string::npos && npos == std::string::npos) {
        // find \r only
        if(rpos == m_readBuffer.size()-1) {
            m_lineState = LineState::LINE_OPEN;
        }
        else {
            m_lineState = LineState::LINE_BAD;
        }
    }
    else if(rpos == std::string::npos && npos != std::string::npos) {
        // find \n only
        m_lineState = LineState::LINE_BAD;
    }
    else {
        // nothing found
        m_lineState = LineState::LINE_OPEN;
    }
    return line;
}
// end of HttpRequest


