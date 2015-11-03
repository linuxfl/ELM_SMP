#
# Makefile for ELM ALG
#
DIR_SRC = ./src
MPI_NODE_SRC = ./mpi_node
DIR_INC = ./include

#c complier
#CC = gcc
#mpi gcc
CC = /home/fangling/mpi/bin/mpicc

INC_CLAS        = /opt/OpenBLAS/include/
LIB_CLAS        = /opt/OpenBLAS/lib/

CFLAGS		= -Wall -I${DIR_INC} -I${INC_CLAS}

LDFLAGS		= -lm -fopenmp

SRC = $(wildcard ${DIR_SRC}/*.c)
NODE_SRC = $(wildcard ${MPI_NODE_SRC}/*.c)

OBJ = $(patsubst %.c,${DIR_SRC}/%.o,$(notdir ${SRC}))
OBJ += $(patsubst %.c,${MPI_NODE_SRC}/%.o,$(notdir ${NODE_SRC}))

TARGET = main

BIN_TARGET = ./${TARGET}

${BIN_TARGET}:${OBJ}
	$(CC) ${OBJ} ${LDFLAGS} -o $@ -L ${LIB_CLAS} -lopenblas

${DIR_SRC}/%.o:${DIR_SRC}/%.c
	 $(CC) $(CFLAGS) -c $< -o $@ -fopenmp

.PHONY:clean
clean:
	rm ${DIR_SRC}/*.o ${DIR_SRC}/*~ -rf
	rm ${DIR_INC}/*~ -rf
	rm *~ -rf


