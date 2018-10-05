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
#include <stdarg.h>

#define MAX_CLIENTS 5
#define BUFFSIZE 2048

const char* INVALID_GUESS_ERROR = "6Error, invalid guess length";

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

bool nameInUse(const struct client* clients, const char* name)
{
	for (int i=0; i<MAX_CLIENTS; ++i)
		if (clients[i].port != -1 && strcmp(clients[i].name, name) == 0)
			return true;
	return false;
}

int correctLetters(const char* secret, const char* guess)
{
	int wordSize = strlen(secret);
	int ans = 0;
	char wordCopy[wordSize];
	strcpy(wordCopy,secret);

	char guessCopy[wordSize];
	strcpy(guessCopy,guess);

	for (int i = 0; i < wordSize; ++i) {
		for (int r = 0; r < wordSize; ++r) {
			if (wordCopy[r] == guessCopy[i] && guessCopy[i] != '\0') {
				wordCopy[r] = '\0';
				guessCopy[i] = '\0';
				++ans;
			}
		}
	}
	return ans;
}

int correctlyPlacedLetters(const char* secret, const char* guess)
{
	int ans = 0;
	for (int i=0; i<strlen(secret); ++i)
		if (secret[i] == guess[i])
			ans++;
	return ans;
}

void sendAll(const struct client* clients, const char* msg, ...)
{
	va_list argptr;
	va_start(argptr, msg);

	char messageToSend[BUFFSIZE];
	vsnprintf(messageToSend, BUFFSIZE, msg, argptr);
	va_end(argptr);

	//printf("FORMATTING: %s\n", msg);
	//printf("SENDING TO ALL: %s\n", messageToSend);

	for (int i=0; i<MAX_CLIENTS; ++i)
		if (clients[i].port != -1)
			send(clients[i].port, messageToSend, strlen(messageToSend)+1, 0);
}

void handleGuess(char* buff, const struct client* clients, const char* secret, const struct client* guesser)
{
	char* guess = buff+1;
	char* lastChar=guess;
	while (*lastChar != '\n')
		++lastChar;
	*lastChar = '\0';	

	if (strlen(guess) != strlen(secret))
		send(guesser->port, INVALID_GUESS_ERROR, strlen(INVALID_GUESS_ERROR)+1, 0);
	else if (strcmp(secret, guess) == 0)
	{
		sendAll(clients, "5%s has correctly guessed the word %s", guesser->name, secret);
		for (int i=0; i<MAX_CLIENTS; ++i)
			shutdown(clients[i].port, SHUT_RDWR);
	}
	else
		sendAll
		(
			clients,
			"7%s guessed %s: %d letter(s) were correct and %d letter(s) were correctly placed",
			guesser->name,
			guess,
			correctLetters(secret, guess),
			correctlyPlacedLetters(secret, guess)
		);
}

int main(int argc, char** argv)
{
	//~verify command line args~
	if (argc != 2) {
	   fprintf(stderr,"Usage: %s <dictFileName>\n", argv[0]);
	   exit(0);
	}
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
	printf("total # words in dict = %d\n",numDictWords);
	srand(time(NULL));
	//char* secret = dictWords[abs((rand() * rand()) % numDictWords)];
	char* secret = "cross";

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
	send(clients[0].port,buff,strlen(buff)+1,0);

	while (true) {
		fd_set rfds;
		fd_set_initialize(clients, &rfds);
		select(max_port(clients)+1, &rfds, NULL, NULL, NULL);

		for (int i=0; i<MAX_CLIENTS; ++i) {
			if (clients[i].port != -1 && FD_ISSET(clients[i].port, &rfds)) {
				read(clients[i].port,buff,BUFFSIZE-1);
				printf("Just received message [%s]\n",buff);
				fflush(stdout);
				
				if (buff[0] == '1')
				{
					//client just sent us their name; check if its in use
					if (nameInUse(clients, buff+1))
						send(clients[i].port,"2",2,0);
					else
					{
						strcpy(clients[i].name, buff+1);
						char acceptNameMessage[4];
						acceptNameMessage[0] = '3';
						acceptNameMessage[1] = countActivePlayers(clients);
						*((uint16_t*) (acceptNameMessage+2)) = strlen(secret);
						send(clients[i].port, acceptNameMessage, 4, 0);
					}
				}
				else if (buff[0] == '4')
					handleGuess(buff, clients, secret, clients+i);
			}
		}
	}

	//free dynamic memory before quitting
	for (int i = 0; i < MAX_CLIENTS; free(clients[i].name),++i);
	for (int i = 0; i < numDictWords; free(dictWords[i]),++i);
	free(&numDictWords);
}