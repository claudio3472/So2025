#include "funcs.h"
#define SHM_KEY 1234


pthread_t *threads;
int num_threads;
transactions_Pool *trans_pool = NULL;

void *miner_thread() {
    // Access shared memory
    int shmid = shmget(SHM_KEY, sizeof(transactions_Pool), 0777);

    if (shmid == -1) {
        perror("Error: Unable to access shared memory");
        return NULL;
    }

    trans_pool = (transactions_Pool *)shmat(shmid, NULL, 0);
    if (trans_pool == (void *)-1) {
        perror("Error: Unable to attach shared memory");
        return NULL;
    }
    printf("Shared memory attached successfully.\n");

    while (1) {
        //pthread_testcancel();
        
        if (sem_wait(sem_transactions) == -1) {
            perror("sem_wait failed");
            return NULL;
        }
        
        int tx_count = 0;
        int max_trans_per_block = trans_pool->max_trans_per_block;
        int pool_size = trans_pool->pool_size;
        transaction selected[max_trans_per_block];


        bool selected_flags[pool_size];
        memset(selected_flags, 0, sizeof(selected_flags));

        int available_tx_count = 0;
        for (int i = 0; i < pool_size; i++) {
            if (trans_pool->transactions[i].active) {
                available_tx_count++;
            }
        }

        if (available_tx_count < max_trans_per_block) {
            sem_post(sem_transactions); 
            printf("\nNão há transações suficientes, vou dormir e verificar a seguir\n");
            sleep(3);  
            continue;
        }
        

        //gerar um numero de 0 a 2 para alternar entre os 3 modos como vamos selecionar as transactions
        while (tx_count < max_trans_per_block) {
            int random_index = rand() % pool_size;

            if (!selected_flags[random_index] && trans_pool->transactions[random_index].active) {
                selected[tx_count] = trans_pool->transactions[random_index];  
                selected_flags[random_index] = 1; 
                tx_count++; 
            }
        }
        
        sem_post(sem_transactions);

        //bloco
        block new_block;
        new_block.block_id = rand() % 1000000;  //id atoa
        //new_block.miner_id = miner_id;
        new_block.num_transactions = tx_count;
        new_block.timestamp = time(NULL);
        memcpy(new_block.transactions, selected, sizeof(transaction) * tx_count);
        printf("Miner meteu %d transações dentro de um bloco\n", tx_count);

        
        //fazer  poW e enviar para o validator pela pipe mas isso é um assunto delicado ahahah
        
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