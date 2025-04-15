#include "funcs.h"

void cleanall(){
    logwrite("Validator thread closed. Exiting.\n");
    exit(0);
}

int validator(){

    //falta semopen e assim aqui




    //aqui vamos receber o bloco pela pipe e verificar se todas as transações que estão no bloco ainda estão na memoria e se tiver todas
    //ele valida o bloco e mete na outra memoria, senão ele descarta o bloco pois da maneira que estamos a fazer vamos manter as transações na memoria mesmo
    //que ela ja esteja num bloco, secalhar discutir a possibilidade de adicionar uma variavel a dizer que já foi pega por um miner????

    //não percebi um cu de como o valiador vai verificar o Pow do hash do sla oq com o do anterior mas dps vê-se
    
    logwrite("Validator thread started\n");
    signal(SIGINT, cleanall);
    pause();
    return 0;
}