#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdbool.h>
#include <string.h>
#include <sys/select.h>

#include "opcodes.h"

#define BUFFSIZE 2048

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
attempt to connect to the specified port #
@param port: the port to try to connect to
@return the socket corresponding to our new connection 
*/
int connectToPort(int port)
{
    int ans = socket(AF_INET, SOCK_STREAM, 0);
    if (ans == 0)
        errorFailure("Socket creation failed");
    
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
        errorFailure("Invalid address");
    
    if (connect(ans, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        errorFailure("Connecting failed");
    
    return ans;
}

void handleClientInput(int sock, bool requestingName)
{
	char buff[BUFFSIZE];
	fgets(buff+1, BUFFSIZE-1, stdin);
	buff[strlen(buff+1)] = '\0';
	buff[0] = requestingName ? SEND_NAME : SEND_GUESS;
	send(sock, buff, strlen(buff)+1, 0);
}

void handleServerMessage(int sock, bool* requestingName)
{
	char buff[BUFFSIZE];
	if (read(sock,buff,BUFFSIZE) == 0)
		exit(0);
	if (buff[0] == REQ_NAME)
	{
		*requestingName = true;
		printf("Please enter your username:\n");
	}
	else if (buff[0] == ACK_NAME)
	{
		*requestingName = false;
		uint16_t keyLength = *((uint16_t*) (buff+2));
    	printf
		(
			"Looks like I'm playing a game with %d player%s and a secret word of length %d\n",
			buff[1],
			buff[1] == 1 ? "" : "s",
			keyLength
		);
	}
	else
	{
		printf("%s\n", buff+1);
		exit(0);
	}
}

int main(int argc, char** argv) {
	//~verify command line args~
    if (argc != 2) {
       fprintf(stderr,"Usage: %s <port>\n", argv[0]);
       exit(0);
    }

	int sock = connectToPort(atoi(argv[1]));
	bool requestingName = false;
	while (true)
	{
		fd_set rfds;
    	FD_ZERO(&rfds);
    	FD_SET(STDIN_FILENO,&rfds);
    	FD_SET(sock,&rfds);
		select(sock+1, &rfds, NULL, NULL, NULL);

		if (FD_ISSET(STDIN_FILENO, &rfds))
			handleClientInput(sock, requestingName);
		if (FD_ISSET(sock, &rfds))
			handleServerMessage(sock, &requestingName);
	}
}