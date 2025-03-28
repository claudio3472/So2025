#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "funcs.h"

#define BUFFER 500
#define SHM_SIZE 2048
#define key1 1234
#define key2 4321
int shmid,shmid2 key, finish;
transactions_Pool trans_Pool;
blockchain_Ledger ledger;
pid_t pid1, pid2, pid3;

void clean(){
    printf("\nSIGINT detected. Cleaning up...\n");
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);
	shmctl(shmid, IPC_RMID, NULL); //elimina memoria partilhada 1
    shmctl(shmid2, IPC_RMID, NULL); //elimina memoria partilhada 2
}

int main(int argc, char *argv[]) {
    char fich[BUFFER];
    if(argc != 2){
        printf("comando: %s {config-file}\n", argv[0]);
        return 1;
    }
    strcpy(fich, argv[1]);

    int NUM_MINERS, TX_POOL_SIZE, TRANSACTIONS_BLOCK, BLOCKCHAIN_BLOCKS;
    finish = 0;
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

    


    // Shared memory for Transaction Pool
    if ((shmid = shmget(KEY1, SHM_SIZE, IPC_CREAT | 0777)) == -1) {
        perror("Error: in shmget - transactions_Pool");
        return -1;
    }
    trans_Pool = (transactions_Pool *)shmat(shmid, NULL, 0);
    if (trans_Pool == -1) { // o stor tinha um tipo antes do -1 mas n faço minima ideia o que era ahahaha
        perror("Error: in shmat - transactions_Pool");
        return -1;
    }

    // Shared memory for Blockchain Ledger
    if ((shmid2 = shmget(KEY2, SHM_SIZE, IPC_CREAT | 0777)) == -1) {
        perror("Error: in shmget - blockchain_Ledger");
        return -1;
    }
    ledger = (blockchain_Ledger *)shmat(shmid2, NULL, 0);
    if (ledger == -1) { // o stor tinha um tipo antes do -1 mas n faço minima ideia o que era ahahaha
        perror("Error: in shmat - blockchain_Ledger");
        return -1;
    }

    trans_Pool.max_size = TX_POOL_SIZE; //numero maximo de blocos que a pool de transações pode ter
    ledger.max_size = BLOCKCHAIN_BLOCKS;  //numero maximo de blocos que o ledger vai poder ter



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