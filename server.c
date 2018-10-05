#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdbool.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>
#include <sys/types.h>

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
    for (int i=0; i<MAX_CLIENTS; ++i)
        if (clients[i].port != -1)
            FD_SET(clients[i].port, set);
}

/**
count the number of players with an active connection
@param clients: the array of client structs who we wish to count for activity
@return the number of currently active players
*/
int countActivePlayers(const struct client* clients) {
	int sum = 0;
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if (clients[i].port != -1) {
			++sum;
		}
	}
	return sum;
}

int main(int argc, char** argv)
{
	//load words
	char** dictWords = NULL;
	int numDictWords = 0;
	FILE *fp = fopen(argv[1],"r");
    char wordBuff[BUFFSIZE];
    while( fscanf(fp, "%s", wordBuff) != EOF ) {
    	++numDictWords;
        dictWords = (char**)realloc(dictWords, (numDictWords)*sizeof(*dictWords));
        dictWords[numDictWords-1] = (char*)malloc(sizeof(wordBuff));
    strcpy(dictWords[numDictWords-1], wordBuff);
    }
    printf("total # words = %d\n",numDictWords);
    srand(time(NULL));
    int secretWordIndex = abs((rand() * rand()) % numDictWords);
    printf("secret word index = %d\n",secretWordIndex); 

	//todo: replace me with the length of the secret word

	char buff[BUFFSIZE];

	struct client clients[MAX_CLIENTS];
	for (int i = 0; i < MAX_CLIENTS; clients[i].port = -1, clients[i].name = malloc(BUFFSIZE*sizeof(char)), ++i);

	int connection_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (connection_socket < 0)
		errorFailure("Socket creation failed");
	
	struct sockaddr_in servaddr;
	socklen_t s = sizeof(servaddr);
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
	
	//greet the newly connected player and give them a temp name
	buff[0] = '1';
	buff[1] = '\0';
	send(clients[0].port,buff,strlen(buff),0);

	while (true) {
		fd_set rfds;
		fd_set_initialize(clients, &rfds);
		printf("reached before select\n");
		fflush(stdout);
		select(max_port(clients)+1, &rfds, NULL, NULL, NULL);
		printf("reached after select\n");
		fflush(stdout);
		for (int i=0; i<MAX_CLIENTS; ++i) {
			if (clients[i].port != -1 && FD_ISSET(clients[i].port, &rfds)) {
				read(clients[i].port,buff,BUFFSIZE-1);
				printf("Just received message [%s]\n",buff);
				fflush(stdout);
				
				if (buff[0] == '1') {
					//client just sent us their name; check if its in use
					bool nameInUse = false;
					for (int r = 0; r < MAX_CLIENTS; ++r) {
						if (i == r) {
							continue;
						}
						if (clients[r].port != -1 && strcmp(buff+1,clients[r].name) == 0) {
							//name is already in use
							nameInUse = true;
							buff[0] = '2';
							buff[1] = '\0';
							send(clients[i].port,buff,strlen(buff),0);
							break;
						}
					}
					//name is not in use
					if (!nameInUse) {
						strcpy(clients[i].name,buff+1);
						buff[0] = '3';
						buff[1] = countActivePlayers(clients);
						//copy in the secret word length
						sprintf(buff+2, "%d", (int)strlen(dictWords[secretWordIndex]));
						buff[2+sizeof(int)] = '\0';
						fflush(stdout);
						send(clients[i].port,buff,strlen(buff),0);
					}
				}
				else if (buff[0] == '4') {
					//client just sent us a guess
					
				}
			}
		}
	}

	//free dynamic memory before quitting
	for (int i = 0; i < MAX_CLIENTS; free(clients[i].name),++i);
	for (int i = 0; i < numDictWords; free(dictWords[i]),++i);
	free(&numDictWords);
}