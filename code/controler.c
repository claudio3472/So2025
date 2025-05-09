#include "funcs.h"

#define BUFFER 500
#define key1 1234
#define key2 4321


int shmid, shmid2, mqid;
transactions_Pool *trans_Pool;
blockchain_Ledger *ledger;
pid_t pid1, pid2, pid3;
int TRANSACTIONS_PER_BLOCK;
pthread_t thread_validatores_aux;


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
    printf("\nSIGINT detected. Cleaning up...\n");

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);
    
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

    pthread_cancel(thread_validatores_aux);
    pthread_join(thread_validatores_aux, NULL);

    destroy_log_things();
    destroy_trans_sem();
    destroy_block_sem();

    exit(0);
}

/*
void *validator_aux(){
    printf("thread auxiliar para criar validatores criada");
    while(1){
        pthread_testcancel();
    }
    
}*/

int main(int argc, char *argv[]) {

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

    mqid = msgget(IPC_PRIVATE, IPC_CREAT | 0777);

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

    int tam = sizeof(int) * 3 + sizeof(time_t) + sizeof(unsigned int) + HASH_SIZE * 2 + sizeof(transaction) * trans_Pool->max_trans_per_block;

    // Shared memory for Blockchain Ledger
    if ((shmid2 = shmget(key2, BLOCKCHAIN_BLOCKS * sizeof(blockchain_Ledger), IPC_CREAT | 0777)) == -1) {
        perror("Error: in shmget - blockchain_Ledger");
        return 1;
    }
    ledger = (blockchain_Ledger *)shmat(shmid2, NULL, 0);
    if (ledger == (void *)-1) {
        perror("Error: in shmat - blockchain_Ledger");
        return 1;
    }

    //pthread_create(&thread_validatores_aux, NULL, validator_aux, NULL);

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

    pause();
    return 0;
}