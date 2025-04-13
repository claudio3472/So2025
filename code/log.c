#include "funcs.h"

FILE *file;

int init_log_things() {
    sem_log = sem_open("/sem_log", O_CREAT, 0644, 1);
    if (sem_log == SEM_FAILED) {
        perror("sem_open failed");
        return -1;
    }
    
    file = fopen("DEIchain_log.txt", "a");
    if (file == NULL) {
        perror("Error opening file");
        destroy_log_things();
        return -1;
    }
    return 0;
}

int destroy_log_things() {
    if (sem_log != NULL && sem_log != SEM_FAILED) {
        sem_close(sem_log); 
        sem_unlink("/sem_log");
    }

    fclose(file);

    return 0;
}

int logwrite(char* line) {

    if (sem_log == NULL) {
        sem_log = sem_open("/sem_log", 0);
        if (sem_log == SEM_FAILED) {
            perror("sem_open in logwrite failed");
            return -1;
        }
    }

    if (sem_wait(sem_log) == -1) {
        perror("sem_wait failed");
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


    
    sem_post(sem_log);

    return 0;
}
