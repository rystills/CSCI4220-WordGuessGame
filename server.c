#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdbool.h>
#include <string.h>

#define MAX_CLIENTS 5

struct client
{
	int port;
	char* name;
};

void errorFailure(const char* msg)
{
	printf("%s\n", msg);
	exit(EXIT_FAILURE);
}

int max_port(const struct client* clients)
{
	int ans = 0;
	for (int i=0; i<MAX_CLIENTS; ++i)
		if (clients[i].port > ans)
			ans = clients[i].port;
	return ans;
}

int main(int argc, char** argv)
{
	struct client clients[MAX_CLIENTS];
	int max_port = 0;

	int connection_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (connection_socket < 0)
		errorFailure("Socket creation failed");
	
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof servaddr);
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = 0;
	if (bind(connection_socket, (const struct sockaddr *) &servaddr, sizeof servaddr) < 0)
		errorFailure("Bind failed");
	printf("%d\n", servaddr.sin_port);
	
	listen(connection_socket, 1);


	/*
	while (true)
	{
		fd_set rfds;
		fd_set_initialize(clients, &rfds);
		
		select(max_port, &rfds, NULL, NULL, NULL);

		for (int i=0; i<MAX_CLIENTS; ++i)
			if (clients[i].port != -1 && FD_ISSET(clients[i].port, &rfds))
			{
				
			}
	}
	*/
}