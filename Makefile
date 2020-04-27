# Makefile
# Create tsh binary
SRC=main.c
FLAGS=-Wall -Werror -pedantic -Wextra
CC=gcc
BIN=tsh
ZIP=xwilla00.zip

default:
	${CC} ${FLAGS} ${SRC} -o ${BIN}

pack:
	zip -r ${ZIP} ${SRC} Makefile

clean:
	rm -f ${BIN} ${ZIP}

run: default
	./${BIN}

.PHONY: clean run pack default
