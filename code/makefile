CC = gcc
LIBS = -lm
FLAGS = -Wall -pthread -g
PROGS = server client
OBJS = server.o admin_functions.o client.o

all: ${PROGS}

clean:
	rm ${OBJS} *~ ${PROGS}

server: server.o admin_functions.o
	${CC} ${FLAGS} $^ ${LIBS} -o $@

client: client.o admin_functions.o
	${CC} ${FLAGS} $^ ${LIBS} -o $@

%.o: %.c
	${CC} ${FLAGS} -c $< -o $@

##########################

server.o: server.c admin_functions.h

client.o: client.c admin_functions.h

admin_functions.o: admin_functions.c admin_functions.h
