#include "server/server.h"


void run() {
    Server server(10213, 5);
    server.start();
}

int main(void) {
    run();
    return 0;
}