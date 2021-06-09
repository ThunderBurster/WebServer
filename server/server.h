#pragma once

#include "../events_generator/events_generator.h"
#include "../http/http_conn.h"
#include "../threadpool/threadpool.h"
#include "../utils/processable.h"

#include <unordered_map>

// currently not support sql, logger

class Server {
private:
    int m_listenFd;
    int m_port;

    bool m_running;
    bool m_tick;
    
    ThreadPool m_threadPool;
    std::shared_ptr<EventsGenerator> m_pEpoller;  // shared with HttpConn
    std::unordered_map<int, std::shared_ptr<HttpConn>> m_fd2Conn;
public:
    Server(int port, int threadNum);
    ~Server();
    void start();
private:
    void setNonBlocking(int fd);
    bool listenFd();
    void dealWithConn();
    void dealWithError(int fd);
    void dealWithHttp(int fd);
};