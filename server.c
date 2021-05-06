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

#define PORT "5678" // the 19
#define BACKLOG 10 // how 21
// #define BUFSIZ 1000

void sigchld_handler(int s)
{
	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);
	errno = saved_errno;

}
// get sockaddr, IPv4 or
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
 		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char* argv[]) {
	int sockfd, new_fd; // listen on sock_fd, new connection on new_fd 
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP


	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) { 
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	char proto[256];
  socklen_t len = sizeof(proto);

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (getsockopt(sockfd, IPPROTO_TCP, TCP_CONGESTION, proto, &len) != 0)
    {
        perror("getsockopt");
        return -1;
    }

    printf("Current: %s\n", proto);

    strcpy(proto, argv[1]);

    len = strlen(proto);

    if (setsockopt(sockfd, IPPROTO_TCP, TCP_CONGESTION, proto, len) != 0)
    {
        perror("setsockopt");
        return -1;
    }

    len = sizeof(proto);

    if (getsockopt(sockfd, IPPROTO_TCP, TCP_CONGESTION, proto, &len) != 0)
    {
        perror("getsockopt");
        return -1;
    }

    printf("New: %s\n", proto);

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
	    perror("client: bind");
			continue;
		}
		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if(p==NULL){
		fprintf(stderr, "client: failed to bind\n"); exit(1);
	}
	printf("socket binding done\n");
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

	printf("client: waiting for connections...\n");

	int filefd;
	ssize_t read_return;
	char buffer[BUFSIZ];

	filefd = open("recv.txt", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

	printf("file opened\n");
  if (filefd == -1) {
      perror("open");
      exit(EXIT_FAILURE);
  }
	// while(1) { // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

		if (new_fd == -1) {
	    perror("accept");
			// continue;
		}

		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
	  printf("server: got connection from %s\n", s);

    do {
        read_return = recv(new_fd, buffer, BUFSIZ, 0);
        if (read_return == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        if (write(filefd, buffer, read_return) == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }
        printf("%zd\n", read_return);
    } while (read_return > 0);
    close(filefd);
    close(new_fd);
	// }

	return 0;
}