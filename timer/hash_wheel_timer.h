#pragma once

#include <list>
#include <unordered_map>
#include <vector>

#include "../lock/locker.h"
#include "../utils/noncopyable.h"

class HashWheelTimer: public noncopyable{
public:
    const static int CIRCLE_LEN = 512;
private:
    int m_circleIntervalMs;
    Mutex m_mutex;
    std::unordered_map<int, int> m_circle[CIRCLE_LEN];  // fd to round
    std::unordered_map<int, int> m_fd2Block;  // fd to which block
public:
    HashWheelTimer(int intervalMs);
    ~HashWheelTimer();
    bool addFd(int fd, int timeOutMs);
    bool removeFd(int fd);
    std::vector<int> tick();
};