#include <assert.h>
#include <unistd.h>
#include <errno.h>

#include "events_generator.h"


EventsGenerator::EventsGenerator() {
    m_epollFd = epoll_create(1);
    assert(m_epollFd != -1);
}

EventsGenerator::~EventsGenerator() {
    int ret = close(m_epollFd);
    assert(ret != -1);
}

void EventsGenerator::addFd(int fd, uint32_t events) {
    epoll_event event;
    event.data.fd = fd;
    event.events = events;
    epoll_ctl(m_epollFd, EPOLL_CTL_ADD, fd, &event);
}

void EventsGenerator::removeFd(int fd) {
    epoll_ctl(m_epollFd, EPOLL_CTL_DEL, fd, nullptr);
}

void EventsGenerator::modFd(int fd, uint32_t events) {
    epoll_event event;
    event.data.fd = fd;
    event.events = events;
    epoll_ctl(m_epollFd, EPOLL_CTL_MOD, fd, &event);
}


std::vector<epoll_event> EventsGenerator::wait(int maxNum) {
    // timeout -1 to block
    epoll_event events[maxNum];
    std::vector<epoll_event> ret;
    int cnt = epoll_wait(m_epollFd, events, maxNum, -1);
    if(cnt == -1 && errno == EINTR) {
        cnt = 0;
    }
    assert(cnt != -1);
    ret.reserve(cnt);

    for(int i=0; i<cnt; i++) {
        ret.push_back(events[i]);
    }
    // auto RVO, no need to move
    return ret;
}
