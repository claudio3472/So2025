#include "funcs.h"

void cleanb(){
    logwrite("Statistics thread closed. Exiting.\n");
    exit(0);
}


int statistics(){
    logwrite("Statistics thread started\n");

    /*
    Some statistics to provide:
    ● Number of valid blocks submitted by each specific Miner to the Validator
    ● Number of invalid blocks submitted by each Miner to the Validator
    ● Average time to verify a transaction (since received until added to the Blockchain)
    ● Credits of each Miner (Miners earn credits for submitting valid blocks)
    ● Total number of blocks validated (both correct and incorrect)
    ● Total number of blocks in the Blockchain
    */

    signal(SIGINT, cleanb);
    pause();
    return 0;
}