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

#include "multiclient.h"
#include "opcodes.h"

#define BUFFSIZE 2048

 #define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

const char* INVALID_GUESS_ERROR = "\4Error, invalid guess length";

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
initialize an fd set with a list of ports, as well as stdin
@param clients: the array of client structs whose ports we wish to listen to in the fd set
@param set: the fd set to initialize
*/
fd_set selectOnSockets(const struct client* clients, int connection_socket)
{
	fd_set set;
	FD_ZERO(&set);
	FD_SET(connection_socket, &set);
	for (int i=0; i<MAX_CLIENTS; ++i)
		if (clients[i].socket != -1)
			FD_SET(clients[i].socket, &set);
	select(max(max_socket(clients),connection_socket)+1, &set, NULL, NULL, NULL);
	return set;
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

void initialGameSetup(struct client* clients, char** secret)
{
	// Set up clients array
	for (int i = 0; i < MAX_CLIENTS; ++i)
		clients[i].socket = -1;

	//*secret = dictWords[abs((rand() * rand()) % numDictWords)];
	*secret = "gruel";
}

/**
@return whether the game has ended (true) or not (false) 
*/
bool handleGuess(char* guess, struct client* clients, char** secret, const struct client* guesser)
{
	char* lastChar=guess;
	while (*lastChar != '\n')
		++lastChar;
	*lastChar = '\0';

	if (strlen(guess) != strlen(*secret))
		send(guesser->socket, INVALID_GUESS_ERROR, strlen(INVALID_GUESS_ERROR)+1, 0);
	else if (strcmp(*secret, guess) == 0)
	{
		sendAll(clients, "\5%s has correctly guessed the word %s", guesser->name, *secret);
		for (int i=0; i<MAX_CLIENTS; ++i)
			shutdown(clients[i].socket, SHUT_RDWR);
		initialGameSetup(clients, secret);
	}
	else
		sendAll
		(
			clients,
			"\3%s guessed %s: %d letter(s) were correct and %d letter(s) were correctly placed",
			guesser->name,
			guess,
			correctLetters(*secret, guess),
			correctlyPlacedLetters(*secret, guess)
		);
}

void connectPlayer(struct sockaddr_in* servaddr, struct client* clients, int connection_socket)
{
	struct client* client = firstFreeClient(clients);
	// Ignore attempted connection if the game is full
	if (client != NULL)
	{
		unsigned int len = sizeof(struct sockaddr_in);
		client->socket = accept(connection_socket, (struct sockaddr *) servaddr, &len);
		if (client->socket < 0)
			errorFailure("accept() failed");
		
		// Set name to "" to prevent the preexisting garbage name from being tested for validity
		client->name[0] = '\0';
		requestName(client->socket);
	}
}

char** loadDict(const char* filename, int* numWords)
{
	*numWords = 0;
	char** dict = NULL;
	FILE* fp = fopen(filename,"r");
	char wordBuff[BUFFSIZE];
	while (fscanf(fp, "%s", wordBuff) != EOF) {
		++(*numWords);
		dict = realloc(dict, (*numWords)*sizeof(*dict));
		dict[*numWords-1] = malloc(strlen(wordBuff)+1);
		strcpy(dict[*numWords-1], wordBuff);
	}
	return dict;
}

void handleNameChange(const char* name, const struct client* clients, const char* secret, struct client* sender)
{
	if (nameInUse(clients, name))
		requestName(sender->socket);
	else
	{
		strcpy(sender->name, name);
		acceptName(sender->socket, countActivePlayers(clients), strlen(secret));
	}
}

void handleClientMessage(struct client* clients, char* secret, struct client* sender)
{
	char buff[BUFFSIZE];
	//remove client if we get a read value of 0
	if (read(sender->socket,buff,BUFFSIZE-1) == 0) {
		printf("%s (socket %d) has disconnected\n", sender->name, sender->socket);
		close(sender->socket);
		sender->socket = -1;
	}
	else if (buff[0] == SEND_NAME)
		handleNameChange(buff+1, clients, secret, sender);
	else if (buff[0] == SEND_GUESS)
		handleGuess(buff+1, clients, &secret, sender);
}

int initializeListenerSocket(struct sockaddr_in* servaddr)
{
	int connection_socket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (connection_socket < 0)
		errorFailure("Socket creation failed");
	
	socklen_t s = sizeof(*servaddr);
	memset(servaddr, 0, s);
	servaddr->sin_family = AF_INET;
	servaddr->sin_addr.s_addr = INADDR_ANY;
	servaddr->sin_port = 0;
	if (bind(connection_socket, (const struct sockaddr *) servaddr, s) < 0)
		errorFailure("Bind failed");
	
	//check/get sin_port
	getsockname(connection_socket, (struct sockaddr *) servaddr,&s);
	printf("port is: %d\n", ntohs(servaddr->sin_port));
	fflush(stdout);
	
	//begin listening
	listen(connection_socket, 1);

	return connection_socket;
}

int main(int argc, char** argv)
{
	//~verify command line args~
	if (argc != 2) {
	   fprintf(stderr,"Usage: %s <dictFileName>\n", argv[0]);
	   exit(0);
	}
	//load words
	int numDictWords;
	char** dictWords = loadDict(argv[1], &numDictWords);
	printf("total # words in dict = %d\n",numDictWords);
	srand(time(NULL));

	struct sockaddr_in servaddr;
	int connection_socket = initializeListenerSocket(&servaddr);
	struct client clients[MAX_CLIENTS];
	char* secret;
	initialGameSetup(clients, &secret);

	while (true) {
		fd_set rfds = selectOnSockets(clients, connection_socket);

		if (FD_ISSET(connection_socket,&rfds))
			connectPlayer(&servaddr, clients, connection_socket);

		for (int i=0; i<MAX_CLIENTS; ++i)
			if (clients[i].socket != -1 && FD_ISSET(clients[i].socket, &rfds))
				handleClientMessage(clients, secret, clients+i);
	}
}