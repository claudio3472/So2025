#include <stdio.h>
#include <stdbool.h>
#include <time.h>

int logwrite(char* line) {
    
    FILE *file = fopen("log.txt", "a"); // Open the file in append mode

    if (file == NULL) {
        perror("Error opening file");
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

    fclose(file); // Close the file

    return 0;
}
