#ifndef funcs_h
#define funcs_h

#define SEM_NAME "/log_semaphore"

typedef struct{
	int current_block_id;       // ID do bloco atual
    int count;
    //possivelmente vai ser precisa uma struct para transações
} transactions_Pool;

/*
transactions_list: Current transactions awaiting validation. Each entry on this list must
have the following:
■ empty: field indicating whether the position is available or not
■ age: field that starts with zero and is incremented every time the Validator
touches the Transaction Pool. The Transaction Pool size is defined by the
configuration size.
*/

typedef struct
{
	//TBD
    int count; 
    //possivelmente vai ser preciso criar uma struck to tipo blocos com as cenas que vão ser pedidas no enunciado
} blockchain_Ledger;

int miner(int num);

int validator();

int statistics();

int logwrite(char* line);



#endif