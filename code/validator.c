#include "funcs.h"
#define SHM_KEY 1234
#define SHM_KEY2 4321

transactions_Pool *trans_pool_val = NULL;
blockchain_Ledger *ledger_val = NULL;
block *blk;
int msgid;
int sinal = 1;
pthread_t input_thread;

void print_ledger_info() {
    if (!ledger_val) {
        printf("Ledger is NULL.\n");
        return;
    }
    printf("Ledger count: %d\n", ledger_val->count);
    for (int i = 0; i < ledger_val->count; i++) {
        block *blk = &ledger_val->blocos[i];
        printf("Block %d\n Block ID: %d\n Previous Hash: %s\n Block Hash: %s\n",
               i, blk->block_id, blk->previous_hash, blk->hash);
        for (int j = 0; j < blk->num_transactions; j++) {
            transaction *tx = &blk->transactions[j];
            printf(" [%d] ID: %s | Reward: %d | Value: %.2f | Timestamp: %ld \n", 
                j, tx->tx_id, tx->reward, (float)tx->value, tx->timestamp);
        }
    }
    
}
/*
void print_blockchain() {
    if (!ledger_val) {
        printf("Ledger is NULL.\n");
        return;
    }
    sem_wait(print_sem);
    printf("=================== Blockchain Ledger ===================\n");

    for (int i = 0; i < ledger_val->count; i++) {
        block *blk = &ledger_val->blocos[i];
        printf("||---- Block %03d --\n", i);
        printf("Block ID: BLOCK-%d-%d\n", blk->block_id, i);
        printf("Previous Hash:\n%s\n", blk->previous_hash);
        printf("Block Timestamp: %ld\n ", blk->timestamp);
        printf("Nonce: %u\n", blk->nonce);
        printf("Transactions:\n");

        for (int j = 0; j < blk->num_transactions; j++) {
            transaction *tx = &blk->transactions[j];
            printf(" [%d] ID: %s | Reward: %d | Value: %.2f | Timestamp: %ld \n", 
                j, tx->tx_id, tx->reward, (float)tx->value, tx->timestamp);
        }

        printf("----------------------------------------------------------\n");
    }
    sem_post(print_sem);
}*/



void cleanall(){
    //print_blockchain();
    if(blk){
        free(blk);
        blk = NULL;
    }
    if (trans_pool_val) {
    shmdt(trans_pool_val);
    }
    if (ledger_val) {
        shmdt(ledger_val);
    }
    
    pthread_cancel(input_thread);
    
    logwrite("Validator process closed. Exiting.\n");
    exit(0);
}


void cleanextra(){
    if(blk){
        free(blk);
        blk = NULL;
    }
    if (trans_pool_val) {
    shmdt(trans_pool_val);
    }
    
    pthread_cancel(input_thread);
    if (ledger_val) {
        shmdt(ledger_val);
    }
    
    logwrite("Extra validator process closed\n");
    exit(0);
}




void deserialize_block(const unsigned char *buffer, size_t size, block *blk) {
    const unsigned char *p = buffer;

    int block_id, num_transactions;
    int miner_id;
    time_t timestamp;
    unsigned int nonce;

    // Check buffer size before reading fields
    if (size < sizeof(int) * 3 + sizeof(time_t) + sizeof(unsigned int) + 2 * HASH_SIZE) {
        fprintf(stderr, "Buffer size is too small to deserialize block header\n");
        return;
    }
    
    // Deserialize the block header
    memcpy(&block_id, p, sizeof(int)); p += sizeof(int);
    memcpy(&miner_id, p, sizeof(int)); p += sizeof(int);
    memcpy(&num_transactions, p, sizeof(int)); p += sizeof(int);
    memcpy(&timestamp, p, sizeof(time_t)); p += sizeof(time_t);
    memcpy(&nonce, p, sizeof(unsigned int)); p += sizeof(unsigned int);
    memcpy(blk->previous_hash, p, HASH_SIZE); p += HASH_SIZE;
    memcpy(blk->hash, p, HASH_SIZE); p += HASH_SIZE;

    //size_t expected_size = sizeof(int) * 3 + sizeof(time_t) + sizeof(unsigned int) + 2 * HASH_SIZE + num_transactions * sizeof(transaction);
    
    //printf("Expected size: %zu, Provided size: %zu\n", expected_size, size);
    
    // Print block information for debugging
    /*
    sem_wait(print_sem);
    printf("========== Deserialized Block Info ==========\n");
    printf("Block ID     : %d\n", block_id);
    printf("Miner ID     : %d\n", miner_id);
    printf("Transactions : %d\n", num_transactions);
    printf("Timestamp    : %ld (%s)", timestamp, ctime(&timestamp));
    printf("Nonce        : %u\n", nonce);
    printf("Prev Hash    : %s\n", blk->previous_hash);
    printf("Hash         : %s\n", blk->hash);
    printf("===========================================\n");
    
    sem_post(print_sem);*/

    // Validate the number of transactions
    if (num_transactions < 0) {
        fprintf(stderr, "Invalid number of transactions: %d\n", num_transactions);
        return;
    }

    // Allocate memory for the transactions
    transaction *transactions = malloc(num_transactions * sizeof(transaction));
    if (!transactions) {
        fprintf(stderr, "Memory allocation failed for transactions\n");
        return;
    }

    // Check if the buffer has enough space for the transactions
    size_t remaining_size = buffer + size - p;
    if (remaining_size < num_transactions * sizeof(transaction)) {
        fprintf(stderr, "Buffer size is too small for transactions\n");
        free(transactions);
        return;
    }

    // Deserialize each transaction
    for (int i = 0; i < num_transactions; ++i) {
        memcpy(&transactions[i], p, sizeof(transaction));
        p += sizeof(transaction);
    }


    blk->block_id = block_id;
    blk->miner_id = miner_id;
    blk->num_transactions = num_transactions;
    blk->timestamp = timestamp;
    blk->nonce = nonce;

    for (int i = 0; i < blk->num_transactions; ++i) {
        blk->transactions[i] = transactions[i];
    }

    free(transactions);
}

int check_poW(block *blk){
    int poW_correto = 1;
    int maior_recompensa = 0;
    for (int i = 0; i < blk->num_transactions; ++i) {
        if (blk->transactions[i].reward > maior_recompensa){
            maior_recompensa = blk->transactions[i].reward;
        }
    }

    char *hash = blk->hash;
    int num_zeros_poW_correto = maior_recompensa;
    //printf("rec - %d::::::Hash         : %s\n", num_zeros_poW_correto, hash);
    for (int i = 0; i < num_zeros_poW_correto; ++i) {
        if (hash[i] != '0') {
            poW_correto = 0;
            break;
        }
    }

    // Verifica se o próximo caractere após os zeros também é '0'
    if (hash[num_zeros_poW_correto] == '0') {
        poW_correto = 0;
    }

    return poW_correto;


}


void envia_invalido(int miner_id){
    msg mensagem;
    memset(&mensagem, 0, sizeof(mensagem));  // limpa tudo primeiro
    mensagem.tempo_medio = 0;
    mensagem.miner_id = miner_id;
    mensagem.is_valid = 0;
    mensagem.total_reward = 0;
    mensagem.mtype = 1;
               
    if (msgsnd(msgid, &mensagem, sizeof(msg) - sizeof(long), 0) == -1) {
        perror("msgsnd");
        exit(1);
    }
    //printf("Mensagem enviada com sucesso!\n");
}

void handle_ctrl_l() {
    printf("\n[Ctrl+L detected] Calling your custom function...\n");
    // Call your desired function here
    // custom_function();
}

void* listen_for_ctrl_l() {
    struct termios oldt, newt;
    char ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    while (1) {
        read(STDIN_FILENO, &ch, 1);
        if (ch == 12) { 
            handle_ctrl_l();
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return NULL;
}

int validator(int tam){
    logwrite("Validator thread started\n");
    signal(SIGINT, cleanall);
    signal(SIGTERM, cleanextra);
    //int auxxxx = 0;
    pthread_create(&input_thread, NULL, listen_for_ctrl_l, NULL);
    int fd = open("/tmp/VALIDATOR_INPUT", O_RDONLY);
    if (fd == -1) {
        perror("Erro ao abrir o pipe para leitura");
        exit(EXIT_FAILURE);
    }

    printf("Validator: Pipe aberto, à espera de dados...\n");
    print_sem = sem_open("/print_sync", 0);
    if (print_sem == SEM_FAILED) {
        perror("sem_open");
        exit(1);
    }

    key_t key = ftok("teste", 65);
    msgid = msgget(key, 0666);

    if (msgid == -1) {
        perror("msgget");
        exit(1);
    }

    int shmid = shmget(SHM_KEY, sizeof(transactions_Pool), 0777);
    if (shmid == -1) {
        perror("Error: Unable to access shared memory");
        return 0;
    }

    trans_pool_val = (transactions_Pool *)shmat(shmid, NULL, 0);
    if (trans_pool_val == (void *)-1) {
        perror("Error: Unable to attach shared memory");
        return 0;
    }
    /////////////////////////////////////////////////////////////////


    int shmid2 = shmget(SHM_KEY2, sizeof(blockchain_Ledger), 0777);
    if (shmid2 == -1) {
        perror("Error: Unable to access shared memory");
        return 0;
    }
    ledger_val = (blockchain_Ledger *)shmat(shmid2, NULL, 0);
    if (ledger_val == (void *)-1) {
        perror("Error: Unable to attach shared memory");
        return 0;
    }

    //printf("Shared memory attached successfully.\n");
    /*
    for (int i = 0; i < 5; i++) {
        ssize_t r = read(fd, &num, sizeof(num));
        if (r < 0) {
            perror("Erro ao ler do pipe");
            break;
        } else if (r == 0) {
            fprintf(stderr, "Pipe fechado antes de ler tudo\n");
            break;
        }
    }
    */
    //printf("..........%d\n", num);
    

    
    sem_blockchain = sem_open("/sem_blockchain", 1);
    if (sem_blockchain == SEM_FAILED) {
        perror("sem_open for shared memory");
        exit(1);
    }
    while (1 && sinal) {
        //char *endptr;
        char *buffer = malloc(tam);
        if (!buffer) {
            perror("malloc buffer");
            exit(EXIT_FAILURE);
        }

        ssize_t total_read = 0;
          if (flock(fd, LOCK_EX) == -1) {
            perror("Erro ao obter lock do pipe");
            close(fd);
            exit(EXIT_FAILURE);
        }

        
        while (total_read < tam) {
            ssize_t r = read(fd, buffer + total_read, tam - total_read);

            /*
            strtol(buffer, &endptr, 10);
            if (endptr != buffer) {
                continue;
            }*/

            if (errno == EINTR) {
            continue;  // Foi interrompido por um sinal, tenta novamente
            }
            if (r < 0) {
                perror("Erro ao ler do pipe");
                break;
            } else if (r == 0) {
                // Writer closed the pipe
                sinal = 0;
                //printf("Pipe fechado antes de ler tudo (%zd de %d bytes)\n", total_read, tam);
                break;
            }
            total_read += r;
        }

        if (flock(fd, LOCK_UN) == -1) {
            perror("Erro ao liberar lock do pipe");
            free(buffer);
            close(fd);
            exit(EXIT_FAILURE);
        }

        //printf("->%zu\n", total_read);
        
        if (total_read > 0) {
            // Lê e processa os dados recebidos
            //printf("Validator recebeu %zd bytes\n", total_read);

            
            int block_id, num_transactions;
            int miner_id;
            fflush(stdout);
          
            memcpy(&block_id, buffer, sizeof(int));
            memcpy(&miner_id, buffer + sizeof(int), sizeof(int));
            memcpy(&num_transactions, buffer + 2 * sizeof(int), sizeof(int));
            fflush(stdout);
          

            // Alocar a estrutura completa incluindo o array flexível
            blk = malloc(sizeof(block) + sizeof(transaction) * num_transactions);
            if (!blk) {
                perror("malloc blk");
                continue;
            }
            blk->num_transactions = num_transactions;

            
            deserialize_block((unsigned char *)buffer, tam, blk);
            free(buffer);

            int poW_correto = check_poW(blk);

            int aux = 0;

            int count = ledger_val->count;
            
            if (count > 0) {
                char *last_hash_ledger = ledger_val->blocos[count-1].hash;
                if(strcmp(blk->previous_hash,last_hash_ledger) != 0 ){
                    //printf("Hash do bloco não confere com o hash do bloco anterior\n");
                    envia_invalido(blk->miner_id);
                    free(blk);
                    blk = NULL;
                    
                    continue;
                }
            }else{
                if(strcmp(blk->previous_hash,prev_hash) != 0){
                    //printf("Hash do bloco não confere com o hash do bloco anterior\n");
                    envia_invalido(blk->miner_id);
                    free(blk);
                    blk = NULL;
                    continue;
                }
            }
            

            if(poW_correto){
                

                //verificar se nenhuma das transações do bloco são invalidas
                
                for (int j = 0; j < blk->num_transactions; ++j) {
                    const char* id_transacao_atual = blk->transactions[j].tx_id;
                    //ir procurar se ela existe na shared memory
                    for (int i = 0; i < trans_pool_val->pool_size; i++) {
                        if (strcmp(trans_pool_val->transactions[i].tx_id,id_transacao_atual) == 0 && trans_pool_val->transactions[i].empty == 0  ) {
                            aux += 1;
                            break;
                        }
                    }
                }
                
                //auxxxx +=1;
                if(aux != blk->num_transactions){
                    //printf("Bloco com transação já processada\n");
                    envia_invalido(blk->miner_id);
                    free(blk);
                    blk = NULL;
                    continue;
                }


                if (sem_wait(sem_transactions) == -1) {
                    perror("sem_wait failed");
                    free(blk);
                    blk = NULL;
                    return 0;
                }

                for (int j = 0; j < blk->num_transactions; ++j) {
                    const char* id_transacao_atual = blk->transactions[j].tx_id;
                    //ir procurar se ela existe na shared memory
                    for (int i = 0; i < trans_pool_val->pool_size; i++) {
                        if (strcmp(trans_pool_val->transactions[i].tx_id,id_transacao_atual) == 0 && trans_pool_val->transactions[i].empty == 0 ) {
                            trans_pool_val->transactions[i].empty = 1;
                            trans_pool_val->count -= 1;
                        }

                        //para apenas incrementar a age uma vez em cada um
                        if(j == 1){
                            trans_pool_val->transactions[i].age += 1;
                            if(trans_pool_val->transactions[i].age%50 == 0){
                                trans_pool_val->transactions[i].reward +=1;
                            }
                        }
                    }
                }
                sem_post(sem_transactions);

                if (sem_wait(sem_blockchain) == -1) {
                    perror("sem_wait failed");
                    free(blk);
                    blk = NULL;
                    return 0;
                }

                if(ledger_val->count == ledger_val->tam ){
                    //printf("Ledger is full\n");
                    envia_invalido(blk->miner_id);
                    free(blk);
                    continue;
                }


                block *dst_blk = &ledger_val->blocos[ledger_val->count];
                *dst_blk = *blk;
                for (int i = 0; i < blk->num_transactions; i++) {
                    dst_blk->transactions[i] = blk->transactions[i];
                }
                ledger_val->count++;

                //print_ledger_info();
                // Increment the block count in the ledger
            

                //printf("ledgerrrr - %d\n", ledger_val->count);
                //printf("trans max do bloco 1 - %d\n", ledger_val->blocos[0].num_transactions);


                int miner_id = blk->miner_id;
                time_t time_total=0;
                time_t time_taken;
                int total_rewr = 0;
                for (int i = 0; i < blk->num_transactions; ++i) {
                    time_taken = time(NULL) - blk->transactions[i].timestamp;
                    time_total += time_taken;
                    total_rewr += blk->transactions[i].reward;
                }
                time_t tempo_medio = time_total/blk->num_transactions;

                msg mensagem;
                memset(&mensagem, 0, sizeof(mensagem));  // limpa tudo primeiro
                mensagem.tempo_medio = tempo_medio;
                mensagem.miner_id = miner_id;
                mensagem.is_valid = 1;
                mensagem.total_reward = total_rewr;
                mensagem.mtype = 1;
               
                if (msgsnd(msgid, &mensagem, sizeof(msg) - sizeof(long), 0) == -1) {
                    perror("msgsnd");
                    exit(1);
                }
                

                //printf("Mensagem enviada com sucesso!\n");


                sem_post(sem_blockchain);

                //printf("Bloco valido \n\n");


                
            }else{
                //printf("Bloco invalido\n\n");
                envia_invalido(blk->miner_id);
                free(blk);
                blk = NULL;
                continue;
            }
            
        
            free(blk);
            blk = NULL;
        } else if (total_read == 0) {
       
            continue;
        } else {
            perror("Erro ao ler do pipe");
            
            break;
        }
       
        
    }
    

    close(fd);



    //aqui vamos receber o bloco pela pipe e verificar se todas as transações que estão no bloco ainda estão na memoria e se tiver todas
    //ele valida o bloco e mete na outra memoria, senão ele descarta o bloco pois da maneira que estamos a fazer vamos manter as transações na memoria mesmo
    //que ela ja esteja num bloco, secalhar discutir a possibilidade de adicionar uma variavel a dizer que já foi pega por um miner????

    //não percebi um cu de como o valiador vai verificar o Pow do hash do sla oq com o do anterior mas dps vê-se


    //criar um processo filho causo passse um certo threshold
    
    
    pause();
    return 0;
}