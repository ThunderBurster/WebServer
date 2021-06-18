#include "hash_wheel_timer.h"

#include <ctime>

HashWheelTimer::HashWheelTimer(int intervalMs) {
    m_circleIntervalMs = intervalMs;
}

HashWheelTimer::~HashWheelTimer() {
    // empty
}

bool HashWheelTimer::addFd(int fd, int timeOutMs) {
    // to do
}

bool HashWheelTimer::removeFd(int fd) {
    // to do
}

std::vector<int> tick() {
    // to do
}


