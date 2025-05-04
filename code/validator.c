#include "funcs.h"

void cleanall(){
    logwrite("Validator thread closed. Exiting.\n");
    exit(0);
}

int validator(){

    //falta semopen e assim aqui
    char buffer[1024];
    int fd = open("/tmp/VALIDATOR_INPUT", O_RDONLY);
    if (fd == -1) {
        perror("Erro ao abrir o pipe para leitura");
        exit(EXIT_FAILURE);
    }

    printf("Validator: Pipe aberto, à espera de dados...\n");

    while (1) {
        ssize_t bytes_read = read(fd, buffer, sizeof(buffer));
        if (bytes_read > 0) {
            // Lê e processa os dados recebidos
            printf("Validator recebeu %zd bytes\n", bytes_read);

            // Aqui fazes a deserialização, validação, etc.
            // Exemplo: printar o conteúdo como string (se aplicável)
            buffer[bytes_read] = '\0'; // só se for texto
            printf("Conteúdo: %s\n", buffer);

        } else if (bytes_read == 0) {
            // Escritor fechou o pipe
            printf("Validator: Escritor fechou o pipe. A terminar...\n");
            break;
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