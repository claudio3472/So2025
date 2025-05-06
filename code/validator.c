#include "funcs.h"

void cleanall(){
    logwrite("Validator thread closed. Exiting.\n");
    exit(0);
}

int validator(){
    int num;
    //falta semopen e assim aqui
    int fd = open("/tmp/VALIDATOR_INPUT", O_RDONLY);
    if (fd == -1) {
        perror("Erro ao abrir o pipe para leitura");
        exit(EXIT_FAILURE);
    }

    printf("Validator: Pipe aberto, à espera de dados...\n");

    read(fd, &num, sizeof(num));
    printf("..........%d\n", num);

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

        printf("->%zu\n", total_read);
        if (total_read > 0) {
            // Lê e processa os dados recebidos
            printf("Validator recebeu %zd bytes\n", total_read);

            // Aqui fazes a deserialização, validação, etc.
            // Exemplo: printar o conteúdo como string (se aplicável)
            buffer[total_read] = '\0'; // só se for texto
            printf("Conteúdo: %s\n", buffer);

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