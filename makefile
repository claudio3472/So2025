CC = gcc
FLAGS = -Wall -Wextra

SRC = controler.c miner.c validator.c log.c statistics.c
OUT = controler.o

all: ${OUT}

${OUT}: ${SRC}
	${CC} ${FLAGS} ${SRC} -o ${OUT}

clean:
	rm -f ${OUT}
