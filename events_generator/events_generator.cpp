#include <assert.h>
#include <unistd.h>
#include <errno.h>

#include "events_generator.h"
#include "../logger/rlog.h"


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
    int ret = epoll_ctl(m_epollFd, EPOLL_CTL_ADD, fd, &event);
    if(ret == -1) {
        LOG_WARN("add fd %d to epoller failed, with errno %d", fd, errno);
    }
}

void EventsGenerator::removeFd(int fd) {
    int ret = epoll_ctl(m_epollFd, EPOLL_CTL_DEL, fd, nullptr);
    if(ret == -1) {
        LOG_WARN("remove fd %d to epoller failed, with errno %d", fd, errno);
    }
}

void EventsGenerator::modFd(int fd, uint32_t events) {
    epoll_event event;
    event.data.fd = fd;
    event.events = events;
    int ret = epoll_ctl(m_epollFd, EPOLL_CTL_MOD, fd, &event);
    if(ret == -1) {
        LOG_WARN("mod fd %d to epoller failed, with errno %d", fd, errno);
    }
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
