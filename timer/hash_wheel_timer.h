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
    int m_pBlock;
    Mutex m_mutex;
    std::list<std::pair<int, int>> m_circle[CIRCLE_LEN];  // pair: fd->round
    std::unordered_map<int, int> m_fd2Block;  // fd 2 which block
    std::unordered_map<int, std::list<std::pair<int, int>>::iterator> m_fd2It;  // fd to iterator
public:
    HashWheelTimer(int intervalMs);
    ~HashWheelTimer();
    bool addFd(int fd, int timeOutMs);
    bool removeFd(int fd);
    std::vector<int> tick();
    int getIntervalMs();
};