#include "funcs.h"
#define SHM_KEY 1234


pthread_t *threads;
int num_threads;
transactions_Pool *trans_pool = NULL;

typedef unsigned char BYTE;

/*
void calc_sha_256(BYTE hash_out[HASH_SIZE], const void *data, size_t len) {
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data, len);
    SHA256_Final(hash_out, &ctx);
}
*/

// Calculate SHA-256 hash
void calc_sha_256(BYTE hash_out[HASH_SIZE], const void *data, size_t len) {
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();  // Create new context
    if (mdctx == NULL) {
        perror("Error creating EVP_MD_CTX");
        return;
    }

    // Initialize the context for SHA-256
    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) != 1) {
        perror("Error initializing digest context");
        EVP_MD_CTX_free(mdctx);
        return;
    }

    // Update the context with the data to hash
    if (EVP_DigestUpdate(mdctx, data, len) != 1) {
        perror("Error updating digest context");
        EVP_MD_CTX_free(mdctx);
        return;
    }

    // Finalize the hash and store it in hash_out
    unsigned int out_len = SHA256_DIGEST_LENGTH;
    if (EVP_DigestFinal_ex(mdctx, hash_out, &out_len) != 1) {
        perror("Error finalizing digest context");
        EVP_MD_CTX_free(mdctx);
        return;
    }

    EVP_MD_CTX_free(mdctx);  // Free the context
}

// Debug print hash
void print_hash(BYTE hash[SHA256_DIGEST_LENGTH]) {
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n");
}

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
            if (trans_pool->transactions[i].empty == 0) {
                available_tx_count++;
            }
        }

        if (available_tx_count < max_trans_per_block) {
            sem_post(sem_transactions); 
            printf("\nNot enough transactions, sleeping...\n");
            sleep(3);
            continue;
        }

        while (tx_count < max_trans_per_block) {
            int random_index = rand() % pool_size;
            if (!selected_flags[random_index] && trans_pool->transactions[random_index].empty == 0) {
                selected[tx_count] = trans_pool->transactions[random_index];
                selected_flags[random_index] = 1;
                tx_count++;
            }
        }

        sem_post(sem_transactions);

        // Build block
        block new_block;
        new_block.block_id = rand() % 1000000;  
        new_block.num_transactions = tx_count;
        new_block.timestamp = time(NULL);
        memcpy(new_block.transactions, selected, sizeof(transaction) * tx_count);

        // Hash the block content
        calc_sha_256((BYTE *)new_block.hash, selected, sizeof(transaction) * tx_count);
        printf("Miner created a block with %d transactions:\n", tx_count);
        print_hash((BYTE *)new_block.hash);

        // TODO: Send block to validator via pipe or queue
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
