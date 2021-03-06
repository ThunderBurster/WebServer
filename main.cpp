#include "server/server.h"
#include "logger/rlog.h"


void run() {
    Server server(10213, 8, 30);
    server.start();
}

int main(void) {
    LOG_INIT("log", "webserver", 5);
    LOG_INFO("pid: %d", getpid());
    run();
    return 0;
}