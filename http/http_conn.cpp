#include <sys/socket.h>
#include <cstring>

#include "http_conn.h"

HttpConn::HttpConn() {
    m_connFd = -1;
    m_state = HttpState::CLOSE;
}

HttpConn::~HttpConn() {
    this->closeConn();
    m_pEventsGenerator = nullptr;
}

void HttpConn::process() {
    bool tag = true;
    while(tag) {
        switch(m_state) {
            case HttpState::READ_AND_PROCESS:
            this->processRequest(tag);
            break;

            case HttpState::WRITE:
            this->processResponse(tag);
            break;

            case HttpState::ERROR:
            this->closeConn();
            tag = false;
            break;

            case HttpState::CLOSE:
            tag = false;
            break;
        }      
    }
    // no keep alive && case send done
    if(m_response.getState() == ResponseState::DONE && !m_request.getkeepAlive()) {
        this->closeConn();
    }
}

void HttpConn::closeConn() {
    if(close(m_connFd) == 0) {
        // as worker thread and main thread can both close conn
        // make sure of thread safety
        // only one thread will success close
        this->modFdRemove();
        m_connFd = -1;
        m_state = HttpState::CLOSE;
        m_request.init(true);
        m_response.init();
        m_pEventsGenerator = nullptr;
    }
}

bool HttpConn::init(int connFd, std::shared_ptr<EventsGenerator> &pEventsGenerator) {
    if(m_state == HttpState::CLOSE) {
        m_connFd = connFd;
        m_state = HttpState::READ_AND_PROCESS;
        m_request.init(true);
        m_response.init();
        m_pEventsGenerator = pEventsGenerator;
        return true;
    }
    else {
        return false;
    }
}

// private
void HttpConn::modFdInput() {
    if(m_connFd != -1) {
        m_pEventsGenerator->modFd(m_connFd, EPOLLIN | EPOLLET | EPOLLONESHOT | EPOLLRDHUP);
    }
}

void HttpConn::modFdOutput() {
    if(m_connFd != -1) {
        m_pEventsGenerator->modFd(m_connFd, EPOLLOUT | EPOLLET | EPOLLONESHOT | EPOLLRDHUP);
    }
}

void HttpConn::modFdRemove() {
    if(m_connFd != -1) {
        m_pEventsGenerator->removeFd(m_connFd);
    }
}

void HttpConn::processRequest(bool &tag) {
    m_request.readOnce(m_connFd);
    m_request.tryParse();
    RequestState reqState = m_request.getState();
    if(reqState == RequestState::DONE || reqState == RequestState::PARSE_ERROR) {
        m_response.fillResponseBuffer(m_request);
        m_state = HttpState::WRITE;
    }
    else if(reqState == RequestState::RECV_ERROR) {
        m_state = HttpState::ERROR;
    }
    else {
        // keep doing
        tag = false;
        this->modFdInput();
    }
}

void HttpConn::processResponse(bool &tag) {
    m_response.writeOnce(m_connFd);
    ResponseState repState = m_response.getState();
    if(repState == ResponseState::DONE) {
        // request done or parse error will lead to WRITE
        if(m_request.getState() == RequestState::DONE) {
            m_pEventsGenerator->modFd(m_connFd, EPOLLIN | EPOLLET | EPOLLONESHOT | EPOLLRDHUP);
            tag = false;
            m_request.init(false);
            m_response.init();
        }
        else {
            // parse error
            m_state = HttpState::ERROR;
            
        }
    }
    else if(repState == ResponseState::SEND_ERROR) {
        m_state = HttpState::ERROR;
    }
    else {
        // keep doing
        this->modFdOutput();
    }
}






