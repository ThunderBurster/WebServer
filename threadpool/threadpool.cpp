#include <assert.h>

#include "threadpool.h"
#include "../logger/rlog.h"


ThreadPool::ThreadPool(int numThreads) {
    m_numThreads = numThreads;
    m_taskQueue = std::make_shared<std::queue<std::shared_ptr<processable>>>();
    m_queueMutex = std::make_shared<Mutex>();
    m_queueSem = std::make_shared<Sem>();


    for(int i=0; i<m_numThreads; i++) {
        pthread_t tid;
        assert(0 == pthread_create(&tid, nullptr, ThreadPool::loop, this));
        m_tids.push_back(tid);
    }
}

ThreadPool::~ThreadPool() {
    int count = 0;
    while(count < m_numThreads) {
        if(this->push(std::make_shared<threadsuicide>()))
            count ++;
    }
}

bool ThreadPool::push(std::shared_ptr<processable> task) {
    if(m_queueMutex->lock()) {
        m_taskQueue->push(task);
        m_queueMutex->unlock();
        m_queueSem->post();
        return true;
    }
    else {
        return false;
    }
}

void* ThreadPool::loop(void *arg) {
    ThreadPool *p = (ThreadPool*)arg;  // temp raw pointer
    std::shared_ptr<std::queue<std::shared_ptr<processable>>> sp_taskQueue(p->m_taskQueue);
    std::shared_ptr<Mutex> sp_queueMutex(p->m_queueMutex);
    std::shared_ptr<Sem> sp_queueSem(p->m_queueSem);
    pthread_detach(pthread_self());
    while(true) {
        // get the task
        std::shared_ptr<processable> sp_task;
        sp_queueSem->wait();
        sp_queueMutex->lock();
        if(!sp_taskQueue->empty()) {
            sp_task = sp_taskQueue->front();
            sp_taskQueue->pop();
        }
        sp_queueMutex->unlock();
        // do the task
        if(sp_task) {
            sp_task->process();
        }
    }
    return arg;
}