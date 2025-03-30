#include "funcs.h"

void cleanb(){
    logwrite("Statistics thread closed. Exiting.\n");
    exit(0);
}


int statistics(){
    logwrite("Statistics thread started\n");
    signal(SIGINT, cleanb);
    pause();
    return 0;
}