#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "funcs.h"

#define SHM_KEY 1234


int transaction_generator(int argc, char *argv[]) {

    if (argc != 3) {
        printf("comando: %s {reward} {sleep time(ms)}\n", argv[0]);
        return 1;
    }

    int reward = atoi(argv[1]);
    int sleep_time = atoi(argv[2]);

    if (reward < 1 || reward > 3 || sleep_time < 200 || sleep_time > 3000) {
        printf("Erro: reward reward must be between 1 e 3, and sleep time between 200 e 3000 ms.\n");
        return 1;
    }


    // Acessar a memória compartilhada
    int shmid = shmget(SHM_KEY, sizeof(transactions_Pool), 0777);

    if (shmid == -1) { // o stor tinha um tipo antes do -1 mas n faço minima ideia o que era ahahaha
        perror("Error: ao acessar memória compartilhada");
        return 1;
    }

    transactions_Pool *trans_pool = (transactions_Pool *)shmat(shmid, NULL, 0);
    if (tx_pool == -1) { // o stor tinha um tipo antes do -1 mas n faço minima ideia o que era ahahaha
        perror("Erro: ao anexar memória compartilhada");
        return 1;
    }

    shmdt(trans_pool); // sai da memória compartilhada

}
