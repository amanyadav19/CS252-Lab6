/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <netinet/tcp.h>

#define PORT "5678"
#define BACKLOG 10

void sigchld_handler(int s)
{
	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);
	errno = saved_errno;

}

int main(int argc, char* argv[]) {


	int sockfd, new_fd; // listen on sock_fd, new connection on new_fd 
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	// Pass the address info of self at port 5678 to servinfo
	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) { 
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// proto to pass the TCP varient we are using to setsockopt
	char proto[256];
	strcpy(proto, argv[1]);
	socklen_t len = sizeof(proto);

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		// initializing the socket
		if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		// setting the TCP variant to the one provided in command line arguments
	    if (setsockopt(sockfd, IPPROTO_TCP, TCP_CONGESTION, proto, len) != 0)
	    {
	        perror("setsockopt");
	        continue;
	    }

	    // Bind the socket to the address
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
	    	perror("client: bind");
			continue;
		}
		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	// If looped through all the results and were unable to connect to any
	if(p==NULL){
		fprintf(stderr, "client: failed to bind\n"); exit(1);
	}

	// listen on the created socket
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
	  	perror("sigaction");
		exit(1);
	}

	// open the file socket into which we are going to write the data
	int filefd;
	ssize_t read_return;
	char buffer[BUFSIZ];

	filefd = open("recv.txt", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

	if (filefd == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	// wait for incoming connections
	sin_size = sizeof their_addr;
	new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);


	if (new_fd == -1) {
    	perror("accept");
	}
	// we don't need the listener anymore
	close(sockfd);

	// turn to recieve mode to recieve data from the newly created connection
	do {
	    read_return = recv(new_fd, buffer, BUFSIZ, 0);
	    if (read_return == -1) {
	        perror("read");
	        exit(EXIT_FAILURE);
	    }
	    // write the recived data to the file
	    if (write(filefd, buffer, read_return) == -1) {
	        perror("write");
	        exit(EXIT_FAILURE);
	    }
	} while (read_return > 0);

	// close the file socket and the incoming connection socket
	close(filefd);
	close(new_fd);

	return 0;
}