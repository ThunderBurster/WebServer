#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <atomic>

#include "../utils/processable.h"
#include "../events_generator/events_generator.h"
#include "http_request.h"
#include "http_response.h"
#include "../timer/hash_wheel_timer.h"

enum class HttpState {
    READ_AND_PROCESS, WRITE, ERROR, CLOSE
};

enum class HttpAction {
    MOD_INPUT, MOD_OUTPUT, CLOSE_CONN
};


class HttpConn: public processable {
private:
    int m_connFd;
    HttpState m_state;
    HttpAction m_action;
    HttpRequest m_request;
    HttpResponse m_response;
    std::shared_ptr<EventsGenerator> m_pEventsGenerator;
    std::shared_ptr<HashWheelTimer> m_pTimer;
    int m_timeOutS;

    static std::atomic<int> connCount;
public:
    HttpConn();
    ~HttpConn();
    virtual void process();
    void closeConn();
    bool init(int connFd, std::shared_ptr<EventsGenerator> &pEventsGenerator, std::shared_ptr<HashWheelTimer> &pTimer, int timeOutS);
private:
    void modFdInput();
    void modFdOutput();
    void modFdRemove();
    void processRequest(bool &tag);
    void processResponse(bool &tag);
};