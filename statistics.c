#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
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