#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "funcs.h"

void cleanall(){
    logwrite("Validator thread closed. Exiting.\n");
    exit(0);
}

int validator(){
    logwrite("Validator thread started\n");
    signal(SIGINT, cleanall);
    pause();
    return 0;
}