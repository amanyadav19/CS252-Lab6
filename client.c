/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <netinet/tcp.h>
#include <sys/time.h>


#define PORT "5678" // the port client will be connecting to
#define MAXDATASIZE 10000 // max number of bytes we will send


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// function to calculate the size of a file in bytes
long int findSize(char file_name[])
{
    // opening the file in read mode
    FILE* fp = fopen(file_name, "r");
  
    // checking if the file exist or not
    if (fp == NULL) {
        printf("File Not Found!\n");
        return -1;
    }
  
    fseek(fp, 0L, SEEK_END);
  
    // calculating the size of the file
    long int res = ftell(fp);
  
    // closing the file
    fclose(fp);
  
    return res;
}

int main(int argc, char *argv[]) {

	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	// Pass the address info of localhost at port 5678 to servinfo
	if ((rv = getaddrinfo("localhost", PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// proto to pass the TCP varient we are using to setsockopt
	char proto[256];
	strcpy(proto, argv[1]);
  	socklen_t len = sizeof(proto);


	// loop through all the results and connect to the first we can
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

	    // connect to the socket created
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
	    	perror("client: connect");
			continue;
		}
		break;
	}

	// If looped through all the results and were unable to connect to any
	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n"); 
		return -1;
	}


	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
	printf("client: connecting to %s\n", s);
	freeaddrinfo(servinfo); // all done with this structure

	// open the file socket from where we are going to read the data
	int filefd;
	filefd = open("send.txt", O_RDONLY);

  if (filefd == -1) {
      perror("open");
      exit(EXIT_FAILURE);
  }
struct timeval t1, t2;
	while (1) {
    // Read data into buffer.  We may not have enough to fill up buffer, so we
    // store how many bytes were actually read in bytes_read.
    int bytes_read = read(filefd, buf, sizeof(buf));
    if (bytes_read == 0) // We're done reading from the file
        break;
    
		if (bytes_read < 0) {
			// handle errors
		}
		// printf("%s\n", buf);
		// You need a loop for the write, because not all of the data may be written
		// in one call; write will return how many bytes were written. p keeps
		// track of where in the buffer we are, while we decrement bytes_read
		// to keep track of how many bytes are left to write.
		void *buf_temp = buf;
		
  	gettimeofday(&t1, NULL);
		while (bytes_read > 0) {
			int bytes_written = send(sockfd, buf_temp, bytes_read, 0);
			if (bytes_written <= 0) {
				// handle errors
			}
			printf("%d\n",bytes_written);
      bytes_read -= bytes_written;
      p += bytes_written;
		}
		gettimeofday(&t2, NULL);
		printf("seconds : %ld\n", 
    t2.tv_sec - t1.tv_sec);
	}
	// if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
	// 	perror("recv");
	// 	exit(1);
	// }
	// buf[numbytes] = '\0';
	// printf("client: received '%s'\n",buf);
	char file_name[] = { "recv.txt" };
  long int res = findSize(file_name);
  if (res != -1)
      printf("Size of the file is %ld bytes \n", res);
  printf("thoughput: %f", (float)res/(t2.tv_sec-t1.tv_sec));
	close(sockfd);
	return 0;
}
