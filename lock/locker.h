#pragma once

#include <pthread.h>
#include <semaphore.h>
#include <assert.h>
#include "../utils/noncopyable.h"

class Sem: public noncopyable{
private:
    sem_t m_sem;
public:
    Sem() {
        sem_init(&m_sem, 0, 0);
    }
    Sem(int initValue) {
        sem_init(&m_sem, 0, initValue);
    }
    ~Sem() {
        sem_destroy(&m_sem);
    }
    bool wait() {
        int ret;
        while((ret = sem_wait(&m_sem)) == -1 && errno == EINTR)
            continue;
        assert(ret != -1);
        return true;
    }
    bool post() {
        int ret;
        while((ret = sem_post(&m_sem)) == -1 && errno == EINTR)
            continue;
        assert(ret != -1);
        return true;
    }
};

class Mutex: public noncopyable {
private:
    pthread_mutex_t m_mutex;
public:
    Mutex() {
        pthread_mutex_init(&m_mutex, nullptr);
    }
    ~Mutex() {
        pthread_mutex_destroy(&m_mutex);
    }
    bool lock() {
        return !pthread_mutex_lock(&m_mutex);
    }
    bool unlock() {
        return !pthread_mutex_unlock(&m_mutex);
    }

};