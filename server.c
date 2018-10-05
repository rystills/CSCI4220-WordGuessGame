#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdbool.h>
#include <string.h>
#include <sys/select.h>

#define MAX_CLIENTS 5
#define BUFFSIZE 2048

struct client
{
	int port;
	char* name;
};

/**
display an error message and exit the application
@param msg: the error message to display
*/
void errorFailure(const char* msg)
{
    printf("Error: %s\n", msg);
    exit(EXIT_FAILURE);
}

/**
determine the port with the largest int value
@param clients: the array of client structs whose ports we wish to check
@return the int value of the largest port number in the provided array of ports
*/
int max_port(const struct client* clients)
{
	int ans = 0;
	for (int i=0; i<MAX_CLIENTS; ++i)
		if (clients[i].port > ans)
			ans = clients[i].port;
	return ans;
}

/**
initialize an fd set with a list of ports, as well as stdin
@param clients: the array of client structs whose ports we wish to listen to in the fd set
@param set: the fd set to initialize
*/
void fd_set_initialize(const struct client* clients, fd_set* set)
{
    FD_ZERO(set);
    FD_SET(STDIN_FILENO,set);
    for (int i=0; i<MAX_CLIENTS; ++i)
        if (clients[i].port != -1)
            FD_SET(clients[i].port, set);
}

int main(int argc, char** argv)
{
	char buff[BUFFSIZE];

	struct client clients[MAX_CLIENTS];
	for (int i = 0; i < MAX_CLIENTS; clients[i].port = -1, ++i);
	int max_port = 0;

	int connection_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (connection_socket < 0)
		errorFailure("Socket creation failed");
	
	struct sockaddr_in servaddr;
	socklen_t s = sizeof servaddr;
	memset(&servaddr, 0, s);
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = 0;
	if (bind(connection_socket, (const struct sockaddr *) &servaddr, s) < 0)
		errorFailure("Bind failed");
	
	//check/get sin_port
	getsockname(connection_socket, (struct sockaddr *) &servaddr,&s);
	printf("port is: %d\n", ntohs(servaddr.sin_port));
	fflush(stdout);
	
	//begin listening
	listen(connection_socket, 1);
	
	//Wait for a client to connect
	unsigned int clntLen = sizeof(servaddr);
	if ((clients[0].port = accept(connection_socket, (struct sockaddr *) &servaddr, &clntLen)) < 0)
		errorFailure("accept() failed");
	
	//greet the newly connected player
	buff[0] = '0';
	buff[1] = '\0';
	send(clients[0].port,buff,sizeof(buff),0);

	while (true) {
		fd_set rfds;
		fd_set_initialize(clients, &rfds);
		select(max_port, &rfds, NULL, NULL, NULL);
		printf("IM ALIVE");
		fflush(stdout);
		for (int i=0; i<MAX_CLIENTS; ++i) {
			if (clients[i].port != -1 && FD_ISSET(clients[i].port, &rfds)) {
				read(clients[i].port,buff,BUFFSIZE-1);
				printf("%s\n",buff);
				fflush(stdout);
			}
		}
	}
}