#include "hash_wheel_timer.h"

#include <ctime>

HashWheelTimer::HashWheelTimer(int intervalMs) {
    m_circleIntervalMs = intervalMs;
    m_pBlock = 0;
}

HashWheelTimer::~HashWheelTimer() {
    // empty
}

bool HashWheelTimer::addFd(int fd, int timeOutMs) {
    int nBlocks = timeOutMs / m_circleIntervalMs;
    int round = nBlocks / CIRCLE_LEN;
    m_mutex.lock();
    // pBlock may change
    // lock after get pos, pBlock tick, a more round time
    int pos = (m_pBlock + nBlocks % CIRCLE_LEN) % CIRCLE_LEN;
    if(!m_fd2Block.count(fd)) {
        m_circle[pos].push_front(std::make_pair(fd, round));
        m_fd2Block[fd] = pos;
        m_fd2It[fd] = m_circle[pos].begin();
        m_mutex.unlock();
        return true;
    }
    else {
        // fd already exist
        m_mutex.unlock();
        return false;
    }
}

bool HashWheelTimer::removeFd(int fd) {
    m_mutex.lock();
    if(m_fd2Block.count(fd)) {
        m_circle[m_fd2Block[fd]].erase(m_fd2It[fd]);
        m_fd2Block.erase(fd);
        m_fd2It.erase(fd);
        m_mutex.unlock();
        return true;
    }
    else {
        m_mutex.unlock();
        return false;
    }
}

std::vector<int> HashWheelTimer::tick() {
    // invoked every m_circleIntervalMs ms
    m_mutex.lock();
    std::vector<int> ret;
    auto it = m_circle[m_pBlock].begin();
    while(it != m_circle[m_pBlock].end()) {
        if(it->second > 0) {
            it->second --;
            it ++;
        }
        else {
            ret.push_back(it->first);
            it = m_circle->erase(it);
        }
    }
    m_mutex.unlock();
    return ret;
}


