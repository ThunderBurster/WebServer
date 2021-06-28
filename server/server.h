#pragma once

#include "../events_generator/events_generator.h"
#include "../http/http_conn.h"
#include "../threadpool/threadpool.h"
#include "../utils/processable.h"
#include "../timer/hash_wheel_timer.h"

#include <unordered_map>

// currently not support sql, logger

class Server {
private:
    int m_listenFd;
    int m_port;

    int m_timeOutS;

    bool m_running;
    bool m_tick;

    const static int MAXFD = 1000;
    
    ThreadPool m_threadPool;
    std::shared_ptr<EventsGenerator> m_pEpoller;  // shared with HttpConn
    std::shared_ptr<HashWheelTimer> m_pTimer;
    std::unordered_map<int, std::shared_ptr<HttpConn>> m_fd2Conn;
public:
    Server(int port, int threadNum, int timeOutS);
    ~Server();
    void start();
private:
    void setNonBlocking(int fd);
    bool listenFd();
    void dealWithConn();
    void dealWithError(int fd);
    void dealWithHttp(int fd);
    bool signalDeal();
    void dealWithSig();
    void dealWithTimeOut();
};