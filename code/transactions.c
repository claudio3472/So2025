#include "funcs.h"

#define SHM_KEY 1234


int tx_counter = 0;

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


void read_shared_memory() {
    int pool_size = trans_pool->count;
    printf("\n--- Reading Shared Memory ---\n");

    for (int i = 0; i < pool_size; i++) {
        if (trans_pool->transactions[i].empty == 0) {
            printf("Transaction %d:\n", i);
            printf("  ID: %s\n", trans_pool->transactions[i].tx_id);
            printf("  Reward: %d\n", trans_pool->transactions[i].reward);
            printf("  Value: %d\n", trans_pool->transactions[i].value);
            printf("  Timestamp: %ld\n", trans_pool->transactions[i].timestamp);
            printf("  Age: %d\n", trans_pool->transactions[i].age);
        }
    }
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

    while (true) {
        if (sem_wait(sem_transactions) == -1) {
            perror("sem_wait failed");
            return -1;
        }
    
        int pool_size = trans_pool->pool_size;

        if(trans_pool->count < pool_size){
            for (int i = 0; i < pool_size; i++) {
                if (trans_pool->transactions[i].empty == 1) {  // Slot is available
        
                    // Generate a unique TX ID: PID + Counter
                    snprintf(trans_pool->transactions[i].tx_id, sizeof(trans_pool->transactions[i].tx_id),
                            "TX_%d_%d", getpid(), tx_counter++);
        
                    trans_pool->transactions[i].reward = reward;
                    trans_pool->transactions[i].value = rand() % 100 + 1;
                    trans_pool->transactions[i].timestamp = time(NULL);
                    trans_pool->transactions[i].age = 0; 
                    trans_pool->transactions[i].empty = 0;
                    trans_pool->count += 1;
        
                    printf("Transaction added at index %d -> ID: %s | Reward: %d | Value: %d | Count total %d\n",
                        i,
                        trans_pool->transactions[i].tx_id,
                        trans_pool->transactions[i].reward,
                        trans_pool->transactions[i].value,
                        trans_pool->count );
                    break;
                }
            }
        }else{
            read_shared_memory();
            printf("Transaction Pool is full. Skipping this transaction.\n");
            sem_post(sem_transactions);
            sleep(sleep_time / 1000);
            continue;
        }
        read_shared_memory();
        sem_post(sem_transactions);
        
        
    }
    


    return 0;
}
