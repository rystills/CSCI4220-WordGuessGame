all: client server	
	
client: client.c	
    gcc -o client.out client.c	
	
server: server.c	
    gcc -o server.out server.c	
