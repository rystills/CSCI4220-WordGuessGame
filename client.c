#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdbool.h>
#include <string.h>
#include <sys/select.h>

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
    
    if (connect(ans, (struct sockaddr *) &serv_addr, sizeof serv_addr) < 0)
        errorFailure("Connecting failed");
    
    return ans;
}

/**
blocking i/o read from the socket, quitting the program if 0 bytes are read
@param sock: the socket from which to read
@param buff: the buffer in which to store the result of our read
*/
void readMayQuit(int sock, char* buff) {
	read(sock,buff,BUFFSIZE-1);
}

int main(int argc, char** argv) {
	//~verify command line args~
    if (argc != 2) {
       fprintf(stderr,"usage: %s <port>\n", argv[0]);
       exit(0);
    }

	//~connect to server~
    int sock = connectToPort(atoi(argv[1]));
    char buff[BUFFSIZE];

	//~blocking i/o wait for server to respond~
    readMayQuit(sock,buff);

    char userName[BUFFSIZE];
	//~loop: askinput for username and send to server~
	while (true) {
		//if we didn't quit, then we know the server asked for our username; grab it from the user and stick it in buff
	    fgets(buff, BUFFSIZE, stdin);
	    //remove the newline from our userName
	    buff[sizeof(buff)] = '\0';
	    //copy our username preemptively
    	strcpy(buff,userName); 
    	//send it to the server for validation
	    send(sock,buff,sizeof(buff),0);

		//~blocking i/o wait for server response. If accepted, break loop. Otherwise, goto askinput~
		readMayQuit(sock,buff);    
		//check for the ok message to break
		if (buff[0] = '0' && buff[1] == 'k' && buff[2] == '\0') break;	
	}

	//~blocking i/o wait for server response -> print #players and secret length, store secret length~
    readMayQuit(sock,buff);
    char keyLengthBuff[16];
    strcpy(buff+1,keyLengthBuff);
    int keyLength;
    sscanf(keyLengthBuff, "%d", &keyLength);
    printf("Looks like I'm playing a game with %d players and a secret word of length %d",buff[0],keyLength);

    while (true) {
    	puts("enter a guess word if you want\n");
    	fflush(stdout);
    	//prepare our fd_set
    	fd_set rfds;
    	FD_ZERO(&rfds);
    	FD_SET(STDIN_FILENO,&rfds);
    	FD_SET(sock,&rfds);
    	//~select: askinput -> send word + newline with same length as secret~
    	select(sock+1, &rfds, NULL, NULL, NULL);
    	if (FD_ISSET(STDIN_FILENO, &rfds)) {
    		fgets(buff, BUFFSIZE, stdin);
    		send(sock,buff,sizeof(buff),0);
    	}

    	//~select: read() -> if 0 bytes, close socket and quit~
    	if (FD_ISSET(sock, &rfds)) {
    		readMayQuit(sock,buff);
    		printf("%s\n",buff);
    		exit(0);
    	}
    }	

}