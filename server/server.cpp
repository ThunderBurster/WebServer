#include "server.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

Server::Server(int port, int threadNum): m_threadPool(threadNum) {
    m_listenFd = -1;
    m_port = port;
    m_running = false;
    m_tick = false;
    m_pEpoller = std::make_shared<EventsGenerator>();
    // now listen
    if(!this->listenFd()) {
        return;
    }
    // ok
    m_running = true;
}


Server::~Server() {
    // to release the resource
    close(m_listenFd);
    m_pEpoller = nullptr;
}

void Server::start() {
    if(!m_running) {
        // something wrong during initization server
        return;
    }
    while(m_running) {
        std::vector<epoll_event> events = m_pEpoller->wait(65535, -1);
        for(epoll_event &event: events) {
            int thisFd = event.data.fd;
            uint32_t thisEvent = event.events;

            if(thisFd == m_listenFd) {
                this->dealWithConn();
            }
            else if(thisEvent & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                this->dealWithError(thisFd);
            }
            else if(thisEvent & EPOLLIN) {
                this->dealWithHttp(thisFd);
            }
            else if(thisEvent & EPOLLOUT) {
                this->dealWithHttp(thisFd);
            }
            else {
                // unexpected events, or to extend
            }
        }

    }
    
}

// private
void Server::setNonBlocking(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}

bool Server::listenFd() {
    // return true on success
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(m_port);

    // listen socket create
    m_listenFd = socket(AF_INET, SOCK_STREAM, 0);
    setNonBlocking(m_listenFd);
    if(m_listenFd == -1) {
        return false;
    }
    // set reuse addr(no linger for nonblock socket)
    int optval = 1;
    if(setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        return false;
    }
    // bind
    if(bind(m_listenFd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        return false;
    }
    // listen
    if(listen(m_listenFd, 5) == -1) {
        return false;
    }
    m_pEpoller->addFd(m_listenFd, EPOLLIN | EPOLLET | EPOLLRDHUP);
    return true;
}  

void Server::dealWithConn() {
    struct sockaddr_in clientAddress;
    socklen_t clientAddrLength = sizeof(clientAddress);
    while(true) {
        int connFd = accept(m_listenFd, (struct sockaddr *)&clientAddress, &clientAddrLength);
        if(connFd < 0) {
            break;
        }
        else {
            if(!m_fd2Conn[connFd]) {
                m_fd2Conn[connFd] = std::make_shared<HttpConn>();
            }
            m_fd2Conn[connFd]->init(connFd, m_pEpoller);
        }
    }

}

void Server::dealWithError(int fd) {
    if(m_fd2Conn[fd]) {
        m_fd2Conn[fd]->closeConn();
    }
}

void Server::dealWithHttp(int fd) {
    if(m_fd2Conn[fd]) {
        m_threadPool.push(m_fd2Conn[fd]);
    }

}