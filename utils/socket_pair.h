#pragma once

#include <sys/socket.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>

class SockPair {
private:
    int fds[2];
    SockPair() {
        assert(socketpair(PF_UNIX, SOCK_STREAM, 0, fds) != -1);
        fcntl(fds[0], F_SETFL, fcntl(fds[0], F_GETFD, 0) | O_NONBLOCK);
        fcntl(fds[1], F_SETFL, fcntl(fds[1], F_GETFD, 0) | O_NONBLOCK);
    }
public:
    ~SockPair() {
        close(fds[0]);
        close(fds[1]);
    }
    static SockPair* getInstance() {
        static SockPair obj;
        return &obj;
    }
    int getFd0() {
        return fds[0];
    }
    int getFd1() {
        return fds[1];
    }
};
