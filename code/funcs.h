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

extern sem_t *sem_transactions;
extern sem_t *sem_blockchain;
extern sem_t *sem_log;
#define HASH_SIZE 100



typedef struct {
    int reward;
    time_t timestamp;
    int active; 
} transaction;

typedef struct {      
    int count;
    int pool_size;
    int max_trans_per_block;                 
    transaction transactions[]; 
} transactions_Pool;

/*
transactions_list: Current transactions awaiting validation. Each entry on this list must
have the following:
■ empty: field indicating whether the position is available or not
■ age: field that starts with zero and is incremented every time the Validator
touches the Transaction Pool. The Transaction Pool size is defined by the
configuration size.
*/
typedef struct {
    int block_id;
    int miner_id;
    int num_transactions;
    char hash[HASH_SIZE];
    time_t timestamp;
    transaction transactions[];  
} block;


typedef struct
{
	//TBD
    int count; 
    int actual_block_id;
    int last_block_id;
    //possivelmente vai ser preciso criar uma struck to tipo blocos com as cenas que vão ser pedidas no enunciado
} blockchain_Ledger;

int miner(int num);

int validator();

int statistics();

int logwrite(char* line);

int init_log_things(); 

int destroy_log_things(); 



#endif