#include "funcs.h"

sem_t *sem_transactions = NULL;
sem_t *sem_blockchain = NULL;
sem_t *sem_log = NULL;
sem_t *print_sem = NULL;
sem_t *sem_write = NULL; 

char prev_hash[HASH_SIZE] = "00006a8e76f31ba74e21a092cca1015a418c9d5f4375e7a4fec676e1d2ec1436";

pthread_mutex_t prev_hash_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
