#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include "funcs.h"

pthread_t *threads;
int num_threads;

void *miner_thread() {
    while (1) {
        pthread_testcancel();
    }
    return NULL;
}

void cleanup() {
    for (int i = 0; i < num_threads; i++) {
        pthread_cancel(threads[i]);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    free(threads);
    logwrite("All miner threads closed. Exiting.\n");
    exit(0);
}

int miner(int num) {
    num_threads = num;
    threads = malloc(num * sizeof(pthread_t));
    if (threads == NULL) {
        perror("Failed to allocate memory for threads");
        return 1;
    }
    
    signal(SIGINT, cleanup);

    for (int i = 0; i < num; i++) {
        pthread_create(&threads[i], NULL, miner_thread, NULL);
        char log_msg[50];
        snprintf(log_msg, sizeof(log_msg), "Miner thread %d started\n", i + 1);
        logwrite(log_msg);
    }
    for (int i = 0; i < num; i++) {
        pthread_join(threads[i], NULL);
    }
    
    return 0;
}