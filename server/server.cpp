#include "server.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>

#include "../utils/socket_pair.h"
#include "../logger/rlog.h"

Server::Server(int port, int threadNum, int timeOutS): m_threadPool(threadNum) {
    m_listenFd = -1;
    m_port = port;

    m_timeOutS = timeOutS;

    m_running = false;
    m_tick = false;

    m_pEpoller = std::make_shared<EventsGenerator>();
    m_pTimer = std::make_shared<HashWheelTimer>(1000);  // 1000ms == 1s
    // timer, set fds
    if(!this->signalDeal()) {
        return;
    }
    // now listen
    if(!this->listenFd()) {
        return;
    }
    // initial http conn objects, from fd 5 to fd max
    for(int i=5; i<=Server::MAXFD; i++) {
        if(!m_fd2Conn[i]) {
            m_fd2Conn[i] = std::make_shared<HttpConn>();
        }
    }

    // ok
    m_running = true;
}


Server::~Server() {
    // to release the resource
    close(m_listenFd);
    m_pEpoller = nullptr;
    m_pTimer = nullptr;
}

void Server::start() {
    if(!m_running) {
        // something wrong during initization server
        return;
    }
    LOG_INFO("server start");
    while(m_running) {
        std::vector<epoll_event> events = m_pEpoller->wait(65535);
        for(epoll_event &event: events) {
            int thisFd = event.data.fd;
            uint32_t thisEvent = event.events;

            if(thisFd == m_listenFd) {
                LOG_INFO("listen events arrived");
                this->dealWithConn();
            }
            else if(thisFd == SockPair::getInstance()->getFd0()) {
                this->dealWithSig();
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
                // unknown events
                LOG_WARN("unknow events");
            }
        }
        // end for
        if(m_tick) {
            this->dealWithTimeOut();
            m_tick = false;
        }
    }
    LOG_INFO("server end");
    
    
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
            LOG_WARN("no new conn from listen, with errno %d", errno);
            break;
        }
        else if(connFd > Server::MAXFD) {
            LOG_WARN("get fd %d too big, close it", connFd);
            std::string busyInfo = "server busy, please wait...";
            send(connFd, busyInfo.c_str(), busyInfo.size(), 0);
            close(connFd);
        }
        else {
            LOG_INFO("get conn fd %d", connFd);
            this->setNonBlocking(connFd);
            if(!m_fd2Conn[connFd]) {
                m_fd2Conn[connFd] = std::make_shared<HttpConn>();
            }
            bool ret = m_fd2Conn[connFd]->init(connFd, m_pEpoller, m_pTimer, m_timeOutS);
            if(!ret) {
                LOG_WARN("initial fd %d to http failed", connFd);
            }
            m_pTimer->addFd(connFd, m_timeOutS*1000);
            m_pEpoller->addFd(connFd, EPOLLIN | EPOLLET | EPOLLONESHOT | EPOLLRDHUP);
        }
    }

}

void Server::dealWithError(int fd) {
    if(m_fd2Conn[fd]) {
        LOG_INFO("conn fd %d error, close it", fd);
        bool ret = m_pTimer->removeFd(fd);
        if(!ret) {
            LOG_WARN("remove timer fd %d failed", fd);
        }
        m_fd2Conn[fd]->closeConn();
    }
}

void Server::dealWithHttp(int fd) {
    if(m_fd2Conn[fd]) {
        bool ret = m_pTimer->removeFd(fd);
        if(!ret) {
            LOG_WARN("remove timer fd %d failed", fd);
        }
        m_threadPool.push(m_fd2Conn[fd]);
    }

}

// set socket signal
void sigHandler(int sig) {
    int saveErr = errno;
    char thisSig = (char)sig;
    send(SockPair::getInstance()->getFd1(), &thisSig, 1, 0);
    errno = saveErr;
}
bool Server::signalDeal() {
    struct sigaction sa;
    // signal alarm
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigHandler;
    sigfillset(&sa.sa_mask);
    if(sigaction(SIGALRM, &sa, NULL) == -1) {
        return false;
    }
    // signal term
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigHandler;
    sigfillset(&sa.sa_mask);
    if(sigaction(SIGTERM, &sa, NULL) == -1) {
        return false;
    }
    // ignore signal pipe
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sigfillset(&sa.sa_mask);
    if(sigaction(SIGPIPE, &sa, NULL) == -1) {
        return false;
    }

    m_pEpoller->addFd(SockPair::getInstance()->getFd0(), EPOLLIN | EPOLLET | EPOLLRDHUP);
    alarm(m_pTimer->getIntervalMs()/1000);
    return true;
}

void Server::dealWithSig() {
    char sig;
    while(true) {
        ssize_t bytes = recv(SockPair::getInstance()->getFd0(), &sig, 1, 0);
        if(bytes == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            else {
                // something wrong with signal sockets
                LOG_ERROR("something wrong with signal sockets");
                m_running = false;
                break;
            }
        }
        else if(bytes == 0) {
            LOG_ERROR("something wrong with signal sockets");
            m_running = false;
            break;
        }
        else {
            // success
            switch(sig) {
                case SIGALRM:
                LOG_INFO("get sig to timer tick");
                m_tick = true;
                break;

                case SIGTERM:
                LOG_INFO("get sig to stop this server");
                m_running = false;
                break;

                default:
                LOG_WARN("unknown sig get %d", sig);
                // unknow sig here
                break;
            }
        }
    }
}

void Server::dealWithTimeOut() {
    std::vector<int> timeOutFds = m_pTimer->tick();
    for(int fd: timeOutFds) {
        LOG_INFO("fd %d timeout", fd);
        m_fd2Conn[fd]->closeConn();  // close and remove epoll fd
    }
    alarm(m_pTimer->getIntervalMs()/1000);
}