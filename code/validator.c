#include "funcs.h"

void cleanall(){
    logwrite("Validator thread closed. Exiting.\n");
    exit(0);
}

void deserialize_block(const unsigned char *buffer, size_t size) {
    const unsigned char *p = buffer;

    int block_id, num_transactions;
    int miner_id;
    time_t timestamp;
    unsigned int nonce;
    char previous_hash[HASH_SIZE];
    char hash[HASH_SIZE];

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
    memcpy(previous_hash, p, HASH_SIZE); p += HASH_SIZE;
    memcpy(hash, p, HASH_SIZE); p += HASH_SIZE;

    size_t expected_size = sizeof(int) * 3 + sizeof(time_t) + sizeof(unsigned int) + 2 * HASH_SIZE + num_transactions * sizeof(transaction);
    
    printf("Expected size: %zu, Provided size: %zu\n", expected_size, size);
    
    // Print block information for debugging
    sem_wait(print_sem);
    printf("========== Deserialized Block Info ==========\n");
    printf("Block ID     : %d\n", block_id);
    printf("Miner ID     : %d\n", miner_id);
    printf("Transactions : %d\n", num_transactions);
    printf("Timestamp    : %ld (%s)", timestamp, ctime(&timestamp));
    printf("Nonce        : %u\n", nonce);
    printf("Prev Hash    : %s\n", previous_hash);
    printf("Hash         : %s\n", hash);
    printf("===========================================\n");
    sem_post(print_sem);

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
    free(transactions);
}

int validator(){
    int num;
    
    int fd = open("/tmp/VALIDATOR_INPUT", O_RDONLY);
    if (fd == -1) {
        perror("Erro ao abrir o pipe para leitura");
        exit(EXIT_FAILURE);
    }

    printf("Validator: Pipe aberto, à espera de dados...\n");

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
    
    //printf("..........%d\n", num);

    while (1) {
        char *endptr;
        char buffer[num];
        ssize_t total_read = 0;
        while (total_read < num) {
            ssize_t r = read(fd, buffer + total_read, num - total_read);

            long value = strtol(buffer, &endptr, 10);
            if (endptr != buffer) {
                continue;
            }


            if (r < 0) {
                perror("Erro ao ler do pipe");
                break;
            } else if (r == 0) {
                // Writer closed the pipe
                fprintf(stderr, "Pipe fechado antes de ler tudo (%zd de %d bytes)\n", total_read, num);
                break;
            }
            total_read += r;
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
            block *blk = malloc(sizeof(block) + sizeof(transaction) * num_transactions);
            if (!blk) {
                perror("malloc blk");
                continue;
            }
            blk->num_transactions = num_transactions;

            
            deserialize_block((unsigned char *)buffer, num);
            
            
            free(blk);
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
    
    logwrite("Validator thread started\n");
    signal(SIGINT, cleanall);
    pause();
    return 0;
}