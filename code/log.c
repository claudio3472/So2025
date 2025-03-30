#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <semaphore.h> 
#include <fcntl.h>       
#include <sys/stat.h>    
#include <pthread.h>     
#include <unistd.h>      
#include "funcs.h"


sem_t *sem = NULL; 

int init_semaphore() {
    if (sem == NULL) { 
        sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
        if (sem == SEM_FAILED) {
            perror("Error opening semaphore");
            return -1;
        }
    }
    return 0;
}

int logwrite(char* line) {
    if (init_semaphore() != 0) {
        return -1;
    }

    if (sem_wait(sem) == -1) {
        perror("Error acquiring semaphore");
        return -1;
    }

    FILE *file = fopen("DEIchain_log.txt", "a");
    if (file == NULL) {
        perror("Error opening file");
        sem_post(sem); 
        sem_close(sem);
        return -1;
    }

    time_t currentTime;
    struct tm *localTime;
    
    time(&currentTime);
    localTime = localtime(&currentTime);

    fprintf(file, "%02d-%02d-%04d %02d:%02d:%02d: %s", 
            localTime->tm_mday, localTime->tm_mon + 1, localTime->tm_year + 1900,
            localTime->tm_hour, localTime->tm_min, localTime->tm_sec, line);

    printf("%02d-%02d-%04d %02d:%02d:%02d: %s", 
            localTime->tm_mday, localTime->tm_mon + 1, localTime->tm_year + 1900,
            localTime->tm_hour, localTime->tm_min, localTime->tm_sec, line);

    fclose(file);


    sem_post(sem);

    return 0;
}
