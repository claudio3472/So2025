#include "funcs.h"

int msgqid;
MinerStats* lista = NULL;  // Começa vazia
int total_miners = 0; // Quantos miners temos salvos
int count = 0;
time_t tempo_total = 0;
int total_blocos_validos = 0;
int total_blocos_invalidos = 0;

void cleanb(){
    logwrite("Statistics thread closed. Exiting.\n");


    printf("\n===== Estatísticas Gerais =====\n");
    printf("Total number of blocks validated (both correct and incorrect): %d\n", count);
    printf("Total number of blocks in the Blockchain (válidos): %d\n", total_blocos_validos);

    if (count > 0) {
        double media_tempo = (double)tempo_total / count;
        printf("Average time to verify a transaction: %.2f\n", media_tempo);
    } else {
        printf("Average time to verify a transaction: N/A (nenhum bloco processado)\n");
    }

    printf("\n===== Estatísticas por Minerador =====\n");
    for (int i = 0; i < total_miners; i++) {
        printf("Miner ID: %d\n", lista[i].miner_id);
        printf("  Valid blocks: %d\n", lista[i].blocos_validos);
        printf("  Invalid blocks: %d\n", lista[i].blocos_invalidos);
        printf("  Total credits (recompensa): %d\n", lista[i].total_recompensa);
        printf("\n");
    }
    exit(0);
}

MinerStats* miner_search(int miner_id) {
    for (int i = 0; i < total_miners; i++) {
        if (lista[i].miner_id == miner_id) {
            return &lista[i]; // Já existe
        }
    }

    // Se não existe, cria um novo
    MinerStats* temp = realloc(lista, (total_miners + 1) * sizeof(MinerStats));
    if (temp == NULL) {
        fprintf(stderr, "Erro de alocação!\n");
        exit(1);
    }
    lista = temp;
    

    lista[total_miners].miner_id = miner_id;
    lista[total_miners].blocos_validos = 0;
    lista[total_miners].blocos_invalidos = 0;
    lista[total_miners].total_recompensa = 0;
    total_miners++;
    
    return &lista[total_miners - 1];
}

void atualizar(int miner_id, int valido, int recompensa) {
    MinerStats* m = miner_search(miner_id);
    if (valido) {
        m->blocos_validos++;
        m->total_recompensa += recompensa;
    } else {
        m->blocos_invalidos++;
    }
}



int statistics(){
    logwrite("Statistics thread started\n");

    key_t key = ftok("teste", 65);
    msgqid = msgget(key, 0666);

    if (msgqid == -1) {
    perror("msgget");
    exit(1);
    }

    
    signal(SIGINT, cleanb);
    while(1){
        msg m;
        if (msgrcv(msgqid, &m, sizeof(msg) - sizeof(long), 0, 0) == -1) {
            perror("msgrcv");
            exit(1);
        }

        printf("Mensagem recebida:\n");
        printf("  Miner ID: %d\n", m.miner_id);
        printf("  Valido: %d\n", m.is_valid);
        printf("  Recompensa: %d\n", m.total_reward);
        printf("  Tempo Médio: %ld\n", m.tempo_medio);

        tempo_total += m.tempo_medio;
        count += 1;
        if(m.is_valid){
            total_blocos_validos +=1;
        }else{
            total_blocos_invalidos +=1;
        }
        atualizar(m.miner_id,m.is_valid,m.total_reward);


        /*
        Some statistics to provide:
        ● Number of valid blocks submitted by each specific Miner to the Validator
        ● Number of invalid blocks submitted by each Miner to the Validator
        ● Average time to verify a transaction (since received until added to the Blockchain)
        ● Credits of each Miner (Miners earn credits for submitting valid blocks)
        */
    }
    
    pause();
    return 0;
}