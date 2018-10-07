#include <sys/socket.h>

#define REQ_NAME 1
#define ACK_NAME 2
#define DEN_GUESS 3
#define ERR_GUESS 4
#define ACK_GUESS 5

#define SEND_NAME 1
#define SEND_GUESS 2

void requestName(int socket)
{
	send(socket, "\1", 2, 0);
}

void acceptName(int socket, int activePlayers, uint16_t secretWordLength)
{
	char acceptNameMessage[4];
	acceptNameMessage[0] = ACK_NAME;
	acceptNameMessage[1] = activePlayers;
	*((uint16_t*) (acceptNameMessage+2)) = secretWordLength;
	send(socket, acceptNameMessage, 4, 0);
}