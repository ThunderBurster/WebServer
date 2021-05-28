#pragma once
#include<pthread.h>

class processable {
public:
    virtual void process() = 0;
    virtual ~processable() = default;
};

class threadsuicide: public processable {
    virtual void process() {
        pthread_exit(nullptr);
    }
};