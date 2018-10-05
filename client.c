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
blocking i/o read from the socket, quitting the program if 0 bytes are read or an error code is returned
@param sock: the socket from which to read
@param buff: the buffer in which to store the result of our read
*/
void readMayQuit(int sock, char* buff) {
	if (read(sock,buff,BUFFSIZE-1) <= 0)
		exit(0);
}

int main(int argc, char** argv) {
	//~verify command line args~
    if (argc != 2) {
       fprintf(stderr,"Usage: %s <port>\n", argv[0]);
       exit(0);
    }

	//~connect to server~
    int sock = connectToPort(atoi(argv[1]));
    char buff[BUFFSIZE];

	//~blocking i/o wait for server to respond with request for name~
    readMayQuit(sock,buff);
    //make sure server actually asked for name (OP 0); anything else is grounds to exit
    if (buff[0] != '0') exit(1);
    char userName[BUFFSIZE];
	//~loop: askinput for username and send to server~
	while (true) {
		//if we didn't quit, then we know the server asked for our username; grab it from the user and stick it in buff
	    buff[0] = '0';
	    printf("Please enter your username\n");
	    fflush(stdout);
	    fgets(buff+1, BUFFSIZE-2, stdin);
	    //remove the newline from our userName
	    buff[sizeof(buff)] = '\0';
	    //copy our username preemptively
    	strcpy(buff+1,userName); 
    	//send it to the server for validation
	    send(sock,buff,sizeof(buff),0);

		//~blocking i/o wait for server response. If accepted, break loop. Otherwise, goto askinput~
		readMayQuit(sock,buff);    
		//check for the ok message to break (OP 1)
		if (buff[0] != '1') break;	
		//check for the retry message to continue loop (OP 2)
		if (buff[0] != '2') exit(1);
	}

	//~blocking i/o wait for server response (OP 3) -> print #players and secret length, store secret length~
    readMayQuit(sock,buff);
    if (buff[0] != '3') exit(1);
    char keyLengthBuff[16];
    strcpy(buff+2,keyLengthBuff);
    int keyLength;
    sscanf(keyLengthBuff, "%d", &keyLength);
    printf("Looks like I'm playing a game with %d players and a secret word of length %d\n",buff[1],keyLength);

    while (true) {
    	puts("Enter a guess word if you want\n");
    	fflush(stdout);
    	//prepare our fd_set
    	fd_set rfds;
    	FD_ZERO(&rfds);
    	FD_SET(STDIN_FILENO,&rfds);
    	FD_SET(sock,&rfds);
    	//~select: askinput -> send word + newline with same length as secret~
    	select(sock+1, &rfds, NULL, NULL, NULL);
    	if (FD_ISSET(STDIN_FILENO, &rfds)) {
    		buff[0] = 4;
    		fgets(buff+1, BUFFSIZE-2, stdin);
    		buff[sizeof(buff)] = '\0';
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