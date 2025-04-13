#include "funcs.h"

void cleanall(){
    logwrite("Validator thread closed. Exiting.\n");
    exit(0);
}

int validator(){

    //falta semopen e assim aqui
    logwrite("Validator thread started\n");
    signal(SIGINT, cleanall);
    pause();
    return 0;
}