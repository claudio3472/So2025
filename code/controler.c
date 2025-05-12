#include "funcs.h"

#define BUFFER 500
#define key1 1234
#define key2 4321

int shmid, shmid2, mqid,tam;
transactions_Pool *trans_Pool;
blockchain_Ledger *ledger;
pid_t pid1, pid2, pid3, pid4, pid5;
int TRANSACTIONS_PER_BLOCK;
pthread_t thread_validatores_aux;
pid_t main_pid;


int init_trans_sem() {
    sem_transactions = sem_open("/sem_transactions", O_CREAT, 0777, 1);
    if (sem_transactions == SEM_FAILED) {
        perror("sem_open failed");
        return -1;
    }
    printf("sem_transactions initialized\n");
    return 0;
}

int destroy_trans_sem() {
    if (sem_transactions != NULL && sem_transactions != SEM_FAILED) {
        sem_close(sem_transactions); 
        sem_unlink("/sem_transactions");
    }
    printf("sem_transactions destroyed\n");
    return 0;
}

int init_block_sem() {
    sem_blockchain = sem_open("/sem_blockchain", O_CREAT, 0777, 1);
    if (sem_blockchain == SEM_FAILED) {
        perror("sem_open failed");
        return -1;
    }
    printf("sem_blockchain initialized\n");
    return 0;
}

int destroy_block_sem() {
    if (sem_blockchain != NULL && sem_blockchain != SEM_FAILED) {
        sem_close(sem_blockchain); 
        sem_unlink("/sem_blockchain");
    }
    printf("sem_blockchain destroyed\n");
    return 0;
}

void clean() {
    
    if (getpid() != main_pid) return;

    printf("\nSIGINT detected. Cleaning up...\n");

    pthread_cancel(thread_validatores_aux);

    if (trans_Pool) {
        shmdt(trans_Pool);
    }

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);
     // Kill pid4 if running
    if (pid4 > 0) {
        kill(pid4, SIGTERM);
        waitpid(pid4, NULL, 0);
        printf("[INFO] Validator60 (pid4) terminated during cleanup.\n");
    }

    // Kill pid5 if running
    if (pid5 > 0) {
        kill(pid5, SIGTERM);
        waitpid(pid5, NULL, 0);
        printf("[INFO] Validator80 (pid5) terminated during cleanup.\n");
    }
    shmctl(shmid, IPC_RMID, NULL); // Remove shared memory 1
    logwrite("Shared Memory 1 deleted\n");
    trans_Pool = NULL; // Set pointer to NULL after deletion
    
    shmctl(shmid2, IPC_RMID, NULL); // Remove shared memory 2
    logwrite("Shared Memory 2 deleted\n");
    ledger = NULL;

    msgctl(mqid, IPC_RMID, NULL);
    logwrite("Message Queue deleted\n");

    if (unlink("/tmp/VALIDATOR_INPUT") == 0) {
        logwrite("Named Pipe VALIDATOR_INPUT deleted\n");
    } else {
        perror("Failed to delete named pipe");
    }

    

    destroy_log_things();
    destroy_trans_sem();
    destroy_block_sem();

    exit(0);
}



void *validator_aux(){
    printf("thread auxiliar para criar validatores criada\n");
    int validator60 = 0;
    int validator80 = 0;

    int shmid = shmget(key1, sizeof(transactions_Pool), 0777);
    if (shmid == -1) {
        perror("Error: Unable to access shared memory");
        return 0;
    }

    trans_Pool = (transactions_Pool *)shmat(shmid, NULL, 0);
    if (trans_Pool == (void *)-1) {
        perror("Error: Unable to attach shared memory");
        return 0;
    }

    while(1){
    
        if(trans_Pool != NULL){
            
            if (sem_wait(sem_transactions) == -1) {
                    perror("sem_wait failed");
                    return NULL;
                } 

            if (trans_Pool->pool_size <= 0) {
                fprintf(stderr, "trans_Pool->pool_size not initialized yet\n");
                continue;
            }
            int size_actual = trans_Pool->count;
            int total_size = trans_Pool->pool_size;
            
            sem_post(sem_transactions);

            if (size_actual >= total_size * 0.6 && validator60 ==0) {
                fflush(stdout);
                validator60 = 1;
                pid4 = fork();
                if (pid4 < 0) {
                    printf("Error: fork not executed correctly\n");
                    return NULL;
                } else if (pid4 == 0) {
                    printf("NEW VALIDATOR CREATED - MEMORY PASS THE 60%% capacity\n");
                    validator(tam); 
                    continue;
                }

            }

            if (size_actual >= total_size * 0.8 && validator80 == 0) {
                validator80 = 1;
                pid5 = fork();
                if (pid5 < 0) {
                    printf("Error: fork not executed correctly\n");
                    return NULL;
                } else if (pid5 == 0) {
                    printf("NEW VALIDATOR CREATED - MEMORY PASS THE 80%% capacity\n");
                    validator(tam); 
                    continue;
                }

            }

            if(size_actual < total_size * 0.4){
                if(validator60){
                    kill(pid4, SIGTERM);
                    waitpid(pid4, NULL, 0);
                    printf("[INFO] Validator60 process terminated — memory below 40%% capacity.\n");
                    validator60 = 0;
                }
                
                if(validator80){
                    kill(pid5, SIGTERM);
                    waitpid(pid5, NULL, 0);
                    printf("[INFO] Validator60 process terminated — memory below 40%% capacity.\n");
                    validator80 = 0;
                }
            }
        }
        //pause();

    }
    
}





int main(int argc, char *argv[]) {

    main_pid = getpid();

    init_log_things();
    init_trans_sem();
    init_block_sem();

    print_sem = sem_open("/print_sync", O_CREAT, 0666, 1);
    sem_write = sem_open("/write_sync", O_CREAT, 0644, 1); 
    if (print_sem == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    char fich[BUFFER];
    if(argc != 2){
        printf("comando: %s {config-file}\n", argv[0]);
        return 1;
    }
    strcpy(fich, argv[1]);

    int NUM_MINERS, TX_POOL_SIZE, TRANSACTIONS_BLOCK, BLOCKCHAIN_BLOCKS;
    // Open the file for reading
    FILE *file = fopen(fich, "r");
    if (file == NULL) {
        printf("Erro: Não foi possível abrir o ficheiro.\n");
        return 1;
    }
    int i;
    for(i = 0; i < 4; i++){
        char linha[BUFFER];
        fgets(linha, BUFFER, file);
        if(i == 0){
            NUM_MINERS = atoi(linha);
        }else if(i == 1){
            TX_POOL_SIZE = atoi(linha);
        }else if(i == 2){
            TRANSACTIONS_BLOCK = atoi(linha);
        }else if(i == 3){
            BLOCKCHAIN_BLOCKS = atoi(linha);
        }
    }

    if(i != 4){
        printf("Erro na leitura do ficheiro!\n");
        exit(-1);
    }
    fclose(file);

    printf("Variables assigned successfully:\n");
    printf("NUM_MINERS = %d\n", NUM_MINERS);
    printf("TX_POOL_SIZE = %d\n", TX_POOL_SIZE);
    printf("TRANSACTIONS_BLOCK = %d\n", TRANSACTIONS_BLOCK);
    printf("BLOCKCHAIN_BLOCKS = %d\n", BLOCKCHAIN_BLOCKS);
    
    key_t key = ftok("teste", 65);
    mqid = msgget(key, IPC_CREAT | 0777);

    if (mqid == -1) {
        printf("msgget didnt process correctly");
        exit(1);
    }else{
        logwrite("Message Queue criada\n");
    }

    if (mkfifo("/tmp/VALIDATOR_INPUT", 0666) == -1) {
        if (errno != EEXIST) {
            perror("Error creating named pipe");
            exit(1);
        }
    } else {
        logwrite("Named Pipe VALIDATOR_INPUT created\n");
    }

    // Shared memory for Transaction Pool

    if ((shmid = shmget(key1, TX_POOL_SIZE * sizeof(transactions_Pool), IPC_CREAT | 0777)) == -1) {
        perror("Error: in shmget - transactions_Pool");
        return 1;
    }
    trans_Pool = (transactions_Pool *)shmat(shmid, NULL, 0);
    if (trans_Pool == (void *)-1) {
        perror("Error: in shmat - transactions_Pool");
        return 1;
    }

    trans_Pool->pool_size = TX_POOL_SIZE;
    trans_Pool->max_trans_per_block = TRANSACTIONS_BLOCK;
    trans_Pool->count = 0;



    for (int i = 0; i < TX_POOL_SIZE; i++) {
        trans_Pool->transactions[i].empty = 1;
        trans_Pool->transactions[i].age = 0;   
    }

    tam = sizeof(int) * 3 + sizeof(time_t) + sizeof(unsigned int) + HASH_SIZE * 2 + sizeof(transaction) * trans_Pool->max_trans_per_block;

    int total_size = sizeof(blockchain_Ledger) 
               + BLOCKCHAIN_BLOCKS * (sizeof(block) + TRANSACTIONS_BLOCK * sizeof(transaction));

    if ((shmid2 = shmget(key2, total_size, IPC_CREAT | 0777)) == -1) {
        perror("Error: in shmget - blockchain_Ledger");
        return 1;
    }
    ledger = (blockchain_Ledger *)shmat(shmid2, NULL, 0);
    if (ledger == (void *)-1) {
        perror("Error: in shmat - blockchain_Ledger");
        return 1;
    }

    pthread_create(&thread_validatores_aux, NULL, validator_aux, NULL);
    

    ledger->count = 0;
    ledger->tam = BLOCKCHAIN_BLOCKS;

    pid1 = fork();
    if (pid1 < 0) {
        printf("Error: fork not executed correctly\n");
        return -1;
    } else if (pid1 == 0) {
        miner(NUM_MINERS);
        exit(0); 
    }

    pid2 = fork();
    if (pid2 < 0) {
        printf("Error: fork not executed correctly\n");
        return -1;
    } else if (pid2 == 0) {
        validator(tam); 
        exit(0); 
    }

    pid3 = fork();
    if (pid3 < 0) {
        printf("Error: fork not executed correctly\n");
        return -1;
    } else if (pid3 == 0) {
        statistics();
        exit(0); 
    }

    

    signal(SIGINT, clean);
    signal(SIGTERM, clean); 


    pause();
    return 0;
}