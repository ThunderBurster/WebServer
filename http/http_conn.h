#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <atomic>

#include "../utils/processable.h"
#include "../events_generator/events_generator.h"
#include "http_request.h"
#include "http_response.h"

enum HttpState {
    READ_AND_PROCESS, WRITE, ERROR, CLOSE
};


class HttpConn: public processable {
private:
    static std::atomic<size_t> countLink;
    int m_connFd;
    HttpState m_state;
    HttpRequest m_request;
    HttpResponse m_response;
    std::shared_ptr<EventsGenerator> m_pEventsGenerator;
public:
    HttpConn();
    ~HttpConn();
    virtual void process();
    void closeConn();
    bool init(int connFd, std::shared_ptr<EventsGenerator> &pEventsGenerator);
    static size_t getLinkCount();
private:
    void modFdInput();
    void modFdOutput();
    void modFdRemove();
    void processRequest(bool &tag);
    void processResponse(bool &tag);
};