#include <sys/socket.h>
#include <cstring>

#include "http_conn.h"

HttpConn::HttpConn() {
    m_connFd = -1;
    m_state = HttpState::CLOSE;
}

HttpConn::~HttpConn() {
    this->closeConn();
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
            tag = false;
            break;

            case HttpState::CLOSE:
            tag = false;
            break;
        }      
    }
    // make sure the last thing to do is to modfd/close conn
    // to make sure 2 threads will not maipulate 1 httpconn at the same time
    switch(m_action) {
        case HttpAction::MOD_INPUT:
        m_pTimer->addFd(m_connFd, m_timeOutS*1000);
        this->modFdInput();
        break;

        case HttpAction::MOD_OUTPUT:
        m_pTimer->addFd(m_connFd, m_timeOutS*1000);
        this->modFdOutput();
        break;

        case HttpAction::CLOSE_CONN:
        m_pTimer->removeFd(m_connFd);
        this->closeConn();
        break;
    }
    
}

void HttpConn::closeConn() {
    this->modFdRemove();
    m_state = HttpState::CLOSE;
    m_request.init(true);
    m_response.init();
    
    int fd = m_connFd;
    m_connFd = -1;
    close(fd);
}

bool HttpConn::init(int connFd, std::shared_ptr<EventsGenerator> &pEventsGenerator, std::shared_ptr<HashWheelTimer> &pTimer, int timeOutS) {
    if(m_state == HttpState::CLOSE) {
        m_connFd = connFd;
        m_state = HttpState::READ_AND_PROCESS;
        m_request.init(true);
        m_response.init();
        m_pEventsGenerator = pEventsGenerator;
        m_pTimer = pTimer;
        m_timeOutS = timeOutS;
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
        // need close conn
        m_state = HttpState::ERROR;
        tag = false;
        m_action = HttpAction::CLOSE_CONN;
    }
    else {
        // need keep reading
        tag = false;
        m_action = HttpAction::MOD_INPUT;
    }
}

void HttpConn::processResponse(bool &tag) {
    m_response.writeOnce(m_connFd);
    ResponseState repState = m_response.getState();
    if(repState == ResponseState::DONE) {
        // request done or parse error will lead to WRITE
        if(m_request.getState() == RequestState::DONE) {
            // if keep alive, ready to input, or close
            tag = false;
            m_action = m_request.getkeepAlive()? HttpAction::MOD_INPUT: HttpAction::CLOSE_CONN;
            m_request.init(false);
            m_response.init();
        }
        else {
            // parse error, close it
            m_state = HttpState::ERROR;
            tag = false;
            m_action = HttpAction::CLOSE_CONN;
        }
    }
    else if(repState == ResponseState::SEND_ERROR) {
        tag = false;
        m_action = HttpAction::CLOSE_CONN;
        m_state = HttpState::ERROR;
    }
    else {
        // keep sending
        tag = false;
        m_action = HttpAction::MOD_OUTPUT;
    }
}






