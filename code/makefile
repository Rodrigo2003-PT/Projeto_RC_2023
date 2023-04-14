CC = gcc
LIBS = -lm
FLAGS = -Wall -pthread -g
PROGS = server
OBJS = server.o admin_functions.o

all: ${PROGS}

clean:
	rm ${OBJS} *~ ${PROGS}

server: server.o admin_functions.o
	${CC} ${FLAGS} $^ ${LIBS} -o $@

%.o: %.c
	${CC} ${FLAGS} -c $< -o $@

##########################

server.o: server.c admin_functions.h

admin_functions.o: admin_functions.c admin_functions.h