#include <stdio.h>

#define BUFFSIZE 2048
#define MAX_CLIENTS 5

struct client
{
	int socket;
	char name[BUFFSIZE];
};

/**
determine the socket with the largest int value
@param clients: the array of client structs whose sockets we wish to check
@return the int value of the largest socket number in the provided array of sockets
*/
int max_socket(const struct client* clients)
{
	int ans = 0;
	for (int i=0; i<MAX_CLIENTS; ++i)
		if (clients[i].socket > ans)
			ans = clients[i].socket;
	return ans;
}

struct client* firstFreeClient(struct client* clients)
{
	for (struct client* client=clients; client<clients+MAX_CLIENTS; ++client)
		if (client->socket == -1)
			return client;
	return NULL;
}

/**
count the number of players with an active connection
@param clients: the array of client structs who we wish to count for activity
@return the number of currently active players
*/
int countActivePlayers(const struct client* clients) {
	int sum = 0;
	for (int i = 0; i < MAX_CLIENTS; ++i)
		if (clients[i].socket != -1)
			++sum;
	return sum;
}

bool nameInUse(const struct client* clients, const char* name)
{
	for (int i=0; i<MAX_CLIENTS; ++i)
		if (clients[i].socket != -1 && strcmp(clients[i].name, name) == 0)
			return true;
	return false;
}

void sendAll(const struct client* clients, const char* msg, ...)
{
	va_list argptr;
	va_start(argptr, msg);

	char messageToSend[BUFFSIZE];
	vsnprintf(messageToSend, BUFFSIZE, msg, argptr);
	va_end(argptr);

	for (int i=0; i<MAX_CLIENTS; ++i)
		if (clients[i].socket != -1)
			send(clients[i].socket, messageToSend, strlen(messageToSend)+1, 0);
}