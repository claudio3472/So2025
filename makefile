# Compiler and flags
CC = gcc
FLAGS = -Wall -Wextra

TRANSACTIONS_SRC = transactions.c
CONTROLER_SRC = controler.c log.c miner.c validator.c statistics.c

TRANSACTIONS_OBJ = transactions.o
CONTROLER_OBJ = controler.o log.o miner.o validator.o statistics.o

TRANSACTIONS_PROG = transactions
CONTROLER_PROG = controler

all: ${TRANSACTIONS_PROG} ${CONTROLER_PROG}

clean:
	rm -f ${TRANSACTIONS_OBJ} ${CONTROLER_OBJ} ${TRANSACTIONS_PROG} ${CONTROLER_PROG}

${TRANSACTIONS_PROG}: ${TRANSACTIONS_OBJ}
	${CC} ${FLAGS} ${TRANSACTIONS_OBJ} -o ${TRANSACTIONS_PROG}

${CONTROLER_PROG}: ${CONTROLER_OBJ}
	${CC} ${FLAGS} ${CONTROLER_OBJ} -o ${CONTROLER_PROG}

%.o: %.c
	${CC} ${FLAGS} -c $< -o $@

# Dependencies for object files
controler.o: funcs.h
transactions.o: funcs.h


