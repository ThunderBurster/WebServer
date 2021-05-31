#pragma once

#include <sys/epoll.h>
#include <vector>

// fd here includes listen fd(r), tcp fd(rw), signal fd(r)
// ET by default
class EventsGenerator {
private:
    int m_epollFd;
public:
    EventsGenerator();
    ~EventsGenerator();
    void addFd(int fd, uint32_t events);
    void removeFd(int fd);
    void modFd(int fd, uint32_t events);
    std::vector<epoll_event> wait(int maxNum, int timeout);
};
