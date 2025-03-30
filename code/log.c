    #include "funcs.h"

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

int logwrite(char* line) {
    pthread_mutex_lock(&log_mutex);

    FILE *file = fopen("DEIchain_log.txt", "a");
    if (file == NULL) {
        perror("Error opening file");
        pthread_mutex_unlock(&log_mutex);
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
    
    pthread_mutex_unlock(&log_mutex);


    return 0;
}
