#include "funcs.h"

#define SHM_KEY 1234

transactions_Pool *trans_pool = NULL;

void clean() {
    if (trans_pool != NULL) {
        if (shmdt(trans_pool) == -1) {
            perror("Error: Failed to detach shared memory");
        } else {
            printf("\nSuccessfully detached shared memory.\n");
        }
    }
    printf("Exiting program...\n");
    exit(0);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, clean);

    if (argc != 3) {
        printf("Usage: %s {reward} {sleep time(ms)}\n", argv[0]);
        return 1;
    }

    int reward = atoi(argv[1]);
    int sleep_time = atoi(argv[2]);


    if (reward < 1 || reward > 3 || sleep_time < 200 || sleep_time > 3000) {
        printf("Error: Reward must be between 1 and 3, and sleep time must be between 200 and 3000 ms.\n");
        return 1;
    }


    if (sem_transactions == NULL) {
        sem_transactions = sem_open("/sem_transactions", 0);
        if (sem_transactions == SEM_FAILED) {
            perror("sem_open in transactions failed");
            return -1;
        }
    }

    // Access shared memory
    int shmid = shmget(SHM_KEY, sizeof(transactions_Pool), 0777);

    if (shmid == -1) {
        perror("Error: Unable to access shared memory");
        return 1;
    }

    trans_pool = (transactions_Pool *)shmat(shmid, NULL, 0);
    if (trans_pool == (void *)-1) {
        perror("Error: Unable to attach shared memory");
        return 1;
    }

    printf("Shared memory attached successfully.\n");

    while(true){
        if (sem_wait(sem_transactions) == -1) {
            perror("sem_wait failed");
            return -1;
        }
        //vamos escrever na shared memory aqui
        printf("Transaction with reward %d added\n", reward);
        sem_post(sem_transactions);
        sleep(sleep_time / 1000);
    }

    


    return 0;
}
