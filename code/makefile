CC = gcc
FLAGS = -Wall -Wextra -g -pthread
LIBS = -lssl -lcrypto


TRANSACTIONS_SRC = transactions.c globals.c
CONTROLER_SRC = controler.c log.c miner.c validator.c statistics.c globals.c

TRANSACTIONS_OBJ = transactions.o globals.o
CONTROLER_OBJ = controler.o log.o miner.o validator.o statistics.o globals.o

TRANSACTIONS_PROG = TxGen
CONTROLER_PROG = controler

all: ${TRANSACTIONS_PROG} ${CONTROLER_PROG}


clean:
	rm -f ${TRANSACTIONS_OBJ} ${CONTROLER_OBJ} ${TRANSACTIONS_PROG} ${CONTROLER_PROG}

${TRANSACTIONS_PROG}: ${TRANSACTIONS_OBJ}
	${CC} ${FLAGS} ${TRANSACTIONS_OBJ} -o ${TRANSACTIONS_PROG} ${LIBS}

${CONTROLER_PROG}: ${CONTROLER_OBJ}
	${CC} ${FLAGS} ${CONTROLER_OBJ} -o ${CONTROLER_PROG} ${LIBS}

%.o: %.c
	${CC} ${FLAGS} -c $< -o $@

# Dependencies for object files
controler.o: funcs.h
transactions.o: funcs.h
