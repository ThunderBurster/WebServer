#include "server/server.h"


int main(void) {
    Server server(10213, 5);
    server.start();
    return 0;
}