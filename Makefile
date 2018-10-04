program: client.o server.o
	gcc -o program client.o server.o

client.o: client.c
	gcc -c client.c

server.o: server.c
	gcc -c server.c