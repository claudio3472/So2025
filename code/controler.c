#include "funcs.h"

#define BUFFER 500
#define key1 1234
#define key2 4321


int shmid, shmid2, mqid;
transactions_Pool *trans_Pool;
blockchain_Ledger *ledger;
pid_t pid1, pid2, pid3;

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

    destroy_log_things();
    exit(0);
}

int main(int argc, char *argv[]) {
    init_log_things();
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
        validator(); 
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