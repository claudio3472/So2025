#ifndef funcs_h
#define funcs_h

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <semaphore.h> 
#include <fcntl.h>       
#include <sys/stat.h>    
#include <pthread.h>     
#include <unistd.h> 
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h> 
#include <signal.h>
#include <sys/msg.h>
#include <openssl/sha.h> 
#include <openssl/evp.h>
#include <arpa/inet.h>
#include <sys/file.h>
#include <termios.h>

#define HASH_SIZE (SHA256_DIGEST_LENGTH * 2 + 1)
extern sem_t *sem_transactions;
extern sem_t *sem_blockchain;
extern sem_t *sem_log;
extern sem_t *sem_write; 
extern sem_t *print_sem;
extern pthread_mutex_t prev_hash_mutex;


extern char prev_hash[HASH_SIZE];

typedef struct {
    char tx_id[32];       
    int reward;          
    int value;            
    time_t timestamp;     
    int age;             
    int empty; 
} transaction;

typedef struct {
    int miner_id;
    int blocos_validos;
    int blocos_invalidos;
    int total_recompensa;
} MinerStats;


typedef struct {
    long mtype;        
    int miner_id;      
    int is_valid;      
    int total_reward;     
    time_t tempo_medio;
} msg;




typedef struct {      
    int count;
    int pool_size;
    int max_trans_per_block;                 
    transaction transactions[]; 
} transactions_Pool;

typedef struct {
    int block_id;
    int miner_id;
    int num_transactions;
    time_t timestamp;
    unsigned int nonce;

    char previous_hash[HASH_SIZE];  
    char hash[HASH_SIZE];               

    transaction transactions[];

} block;


typedef struct {
    char hash[HASH_SIZE];
    double elapsed_time;
    int operations;
    int error;
} PoWResult;


typedef struct
{
	
    int count; 
    int tam;
    block blocos[];
    
} blockchain_Ledger;

int miner(int num);

int validator(int tam);

int statistics();

int logwrite(char* line);

int init_log_things(); 

int destroy_log_things(); 



#endif