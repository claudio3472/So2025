#include "funcs.h"
#define SHM_KEY 1234
#define POW_MAX_OPS 1000000
#define SHA256_DIGEST_LENGTH 32  // Tamanho real do hash SHA-256 em bytes

pthread_t *threads;
int num_threads;
transactions_Pool *trans_pool = NULL;

typedef unsigned char BYTE;

void calc_sha_256(BYTE hash_out[SHA256_DIGEST_LENGTH], const void *data, size_t len) {
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (mdctx == NULL) {
        perror("Error creating EVP_MD_CTX");
        return;
    }

    if (EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL) != 1 ||
        EVP_DigestUpdate(mdctx, data, len) != 1 ||
        EVP_DigestFinal_ex(mdctx, hash_out, NULL) != 1) {
        perror("Error computing digest");
        EVP_MD_CTX_free(mdctx);
        return;
    }

    EVP_MD_CTX_free(mdctx);
}

void compute_block_hash(block *blk, int tx_count, BYTE hash_out[SHA256_DIGEST_LENGTH]) {
    size_t buffer_size = sizeof(int) * 3 + sizeof(time_t) + sizeof(unsigned int) + HASH_SIZE + sizeof(transaction) * tx_count;
    BYTE *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("Failed to allocate memory");
        return;
    }

    size_t offset = 0;
    memcpy(buffer + offset, &blk->block_id, sizeof(int)); offset += sizeof(int);
    memcpy(buffer + offset, &blk->miner_id, sizeof(int)); offset += sizeof(int);
    memcpy(buffer + offset, &blk->num_transactions, sizeof(int)); offset += sizeof(int);
    memcpy(buffer + offset, &blk->timestamp, sizeof(time_t)); offset += sizeof(time_t);
    memcpy(buffer + offset, &blk->nonce, sizeof(unsigned int)); offset += sizeof(unsigned int);
    memcpy(buffer + offset, blk->previous_hash, HASH_SIZE); offset += HASH_SIZE;
    memcpy(buffer + offset, blk->transactions, sizeof(transaction) * tx_count); offset += sizeof(transaction) * tx_count;

    if (offset != buffer_size) {
        fprintf(stderr, "Hashing buffer size mismatch!\n");
        free(buffer);
        return;
    }

    calc_sha_256(hash_out, buffer, buffer_size);
    free(buffer);
}

int hash_meets_difficulty(BYTE hash[SHA256_DIGEST_LENGTH], int difficulty) {
    char hash_hex[SHA256_DIGEST_LENGTH * 2 + 1];

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(&hash_hex[i * 2], "%02x", hash[i]);
    }
    hash_hex[64] = '\0';

    for (int i = 0; i < difficulty; i++) {
        if (hash_hex[i] != '0') return 0;
    }
    return 1;
}

PoWResult proof_of_work(block *block, int tx_count, int transactions_per_block) {
    PoWResult result;
    result.error = 1;
    result.operations = 0;
    result.elapsed_time = 0.0;
    memset(result.hash, 0, HASH_SIZE);

    int difficulty = 0;
    for (int i = 0; i < transactions_per_block; i++) {
        if (block->transactions[i].reward > difficulty) {
            difficulty = block->transactions[i].reward;
        }
    }

    BYTE hash[SHA256_DIGEST_LENGTH];
    clock_t start_time = clock();

    for (unsigned int nonce = 0; nonce < POW_MAX_OPS; nonce++) {
        block->nonce = nonce;
        compute_block_hash(block, tx_count, hash);
        result.operations++;

        if (hash_meets_difficulty(hash, difficulty)) {
            memcpy(result.hash, hash, SHA256_DIGEST_LENGTH);
            result.elapsed_time = (double)(clock() - start_time) / CLOCKS_PER_SEC;
            result.error = 0;

            // Armazena o hash final no bloco em hexadecimal
            for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
                sprintf(&block->hash[i * 2], "%02x", hash[i]);
            }
            block->hash[SHA256_DIGEST_LENGTH * 2] = '\0';  // NULL terminator

            return result;
        }
    }

    result.elapsed_time = (double)(clock() - start_time) / CLOCKS_PER_SEC;
    return result;
}

void print_hash(BYTE hash[SHA256_DIGEST_LENGTH]) {
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        printf("%02x", hash[i]);
    }
    printf("\n");
}

unsigned char *serialize_block(const block *blk, size_t *sz_buf) {
    *sz_buf = sizeof(int) * 3 + sizeof(time_t) + sizeof(unsigned int) + HASH_SIZE + sizeof(transaction) * blk->num_transactions;

    unsigned char *buffer = malloc(*sz_buf);
    if (!buffer) return NULL;

    unsigned char *p = buffer;

    memcpy(p, &blk->block_id, sizeof(int));
    p += sizeof(int);
    memcpy(p, &blk->miner_id, sizeof(int)); 
    p += sizeof(int);
    memcpy(p, &blk->num_transactions, sizeof(int));
    p += sizeof(int);
    memcpy(p, &blk->timestamp, sizeof(time_t)); 
    p += sizeof(time_t);
    memcpy(p, &blk->nonce, sizeof(unsigned int)); 
    p += sizeof(unsigned int);
    memcpy(p, blk->previous_hash, HASH_SIZE); 
    p += HASH_SIZE;

    for (int i = 0; i < blk->num_transactions; ++i) {
        memcpy(p, &blk->transactions[i], sizeof(transaction));
        p += sizeof(transaction);
    }

    return buffer;
}

void *miner_thread() {
    int fd = open("/tmp/VALIDATOR_INPUT", O_WRONLY);
     
    if (fd == -1) {
        perror("open");
        exit(1);
    }

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
    //printf("Shared memory attached successfully.\n");

    int tama = sizeof(int) * 3 + sizeof(time_t) + sizeof(unsigned int) + HASH_SIZE + sizeof(transaction) * trans_pool->max_trans_per_block;
    
    write(fd, &tama, sizeof(int));

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
            //printf("\nNot enough transactions, sleeping...\n");
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

        pthread_mutex_lock(&prev_hash_mutex);

        block new_block;

        strcpy(new_block.previous_hash, prev_hash);  

        new_block.block_id = rand() % 1000000;
        new_block.num_transactions = tx_count;
        memcpy(new_block.transactions, selected, sizeof(transaction) * tx_count);

        PoWResult r;
        do {
            new_block.timestamp = time(NULL);
            r = proof_of_work(&new_block, tx_count, max_trans_per_block);
        } while (r.error == 1);

        // Atualiza prev_hash com o hash gerado
        strcpy(prev_hash, new_block.hash);

        pthread_mutex_unlock(&prev_hash_mutex);

        size_t serialized_block_size = 0;
        unsigned char *serialized_block = serialize_block(&new_block, &serialized_block_size);
        if (serialized_block) {
            /*
            printf("Miner created a block with %d transactions:\n", tx_count);
            printf("Previous Hash: %s\n", new_block.previous_hash);
            printf("Current Hash : %s\n", new_block.hash);
            printf("Nonce:%d \n", new_block.nonce);
            */
            free(serialized_block);
        } else {
            fprintf(stderr, "Failed to serialize the block.\n");
        }

        printf("Sending block of size: %zu\n", serialized_block_size);
            
        ssize_t bytes_written = write(fd, serialized_block, serialized_block_size);
        if (bytes_written < 0) {
            perror("write");
            close(fd);
            exit(1);
        }

        fflush(stdout);
        printf("--------------------------%ld\n", bytes_written);
        
        if ((size_t)bytes_written != serialized_block_size) {
            fprintf(stderr, "Partial write occurred!\n");
            close(fd);
            exit(1);
        }

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
