#include <string.h>
#include <unistd.h> 
#include <fcntl.h>
#include <stdio.h> 
#include <err.h>
#include <errno.h>
#include <sys/socket.h> 
#include <sys/types.h>
#include <stdlib.h> 
#include <netinet/in.h> 
#include <netdb.h>
#include <arpa/inet.h>
#include <sstream>
#include <pthread.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <iostream>
using namespace std;

int guard(int n, char * err) { if (n == -1) { perror(err); exit(1); } return n; }

int main(int argc, char * argv[]) {
//   int listen_fd = guard(socket(PF_INET, SOCK_STREAM, 0), "Could not create TCP socket");
//   printf("Created new socket %d\n", listen_fd);
//   guard(listen(listen_fd, 100), "Could not listen on TCP socket");
//   struct sockaddr_in listen_addr;
//   socklen_t addr_len = sizeof(listen_addr);
//   guard(getsockname(listen_fd, (struct sockaddr *) &listen_addr, &addr_len), "Could not get socket name");
//   printf("Listening for connections on port %d\n", ntohs(listen_addr.sin_port));

	struct sockaddr_in servaddr;
 
    int main_socket = socket(AF_INET, SOCK_STREAM, 0);

	char* port = argv[1];

    // struct timeval tv;
    // tv.tv_sec = 1;
    // tv.tv_usec = 0;
    // setsockopt(main_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
	
	//Create Listen Socket
    bzero( &servaddr, sizeof(servaddr));
 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(atoi(port));
 
    bind(main_socket, (struct sockaddr *) &servaddr, sizeof(servaddr));
	listen (main_socket, 16);
	string fork_error = "Could not fork";
	string recv_error = "Could not recv on TCP connection";
	string shutdown_error = "Could not shutdown TCP connection";
	string tcp_close_error = "Could not close TCP connection";
	string tcp_cant_send = "Could not send to TCP connection";
	for (;;) {
	int conn_fd = accept(main_socket, NULL, NULL);
	printf("Got new connection %d\n", conn_fd);
	if (guard(fork(), (char *) fork_error.c_str()) == 0) {
		pid_t my_pid = getpid();
		printf("%d: forked\n", my_pid);
		char buf[100];
		for (;;) {
		ssize_t num_bytes_received = guard(recv(conn_fd, buf, sizeof(buf), 0), (char *) recv_error.c_str());
		if (num_bytes_received == 0) {
			printf("%d: received end-of-connection; closing connection and exiting\n", my_pid);
			guard(shutdown(conn_fd, SHUT_WR), (char *) shutdown_error.c_str());
			guard(close(conn_fd), (char *) tcp_close_error.c_str());
			exit(0);
		}
		printf("%d: received bytes; echoing\n", my_pid);
		guard(send(conn_fd, buf, num_bytes_received, 0), (char *) tcp_cant_send.c_str());
		printf("%d: echoed bytes; receiving more\n", my_pid);
		}
	} else {
		// Child takes over connection; close it in parent
		close(conn_fd);
	}
	}
	return 0;
}
