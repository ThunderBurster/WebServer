#pragma once

#include <pthread.h>
#include <vector>
#include <queue>
#include <memory>
#include <unistd.h>

#include "../lock/locker.h"
#include "../utils/processable.h"
#include "../utils/noncopyable.h"

class ThreadPool: noncopyable {
private:
    // hold by main thread
    int m_numThreads;
    std::vector<pthread_t> m_tids;
    // hold by main thread and pool threads
    std::shared_ptr<std::queue<std::shared_ptr<processable>>> m_taskQueue;
    std::shared_ptr<Mutex> m_queueMutex;
    std::shared_ptr<Sem> m_queueSem;
    
public:
    ThreadPool(int numThreads);
    ~ThreadPool();
    bool push(std::shared_ptr<processable> task);
private:
    static void* loop(void *arg);
    

};