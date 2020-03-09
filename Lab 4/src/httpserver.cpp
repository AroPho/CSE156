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

void establish_connnection(int sock){
	int numbytes;
	char c;
	string temp = "";
	char buffer[128];
	string result = "";
	FILE* pipe;
	// string error_command = "Command not Found\n\r\n";
	// int result_int;
	try{
		while((numbytes = recv(sock, &c, 1, 0)) != 0){
			temp += c;
			if(temp.length() > (long) 1 && temp.substr(temp.length() - 2) == "\r\n"){
				// determine_command(temp, sock);
				// printf("%s\n", temp.c_str());
				// Open pipe to file
				pipe = popen(temp.substr(0, temp.length() - 2).c_str(), "r");
				if (!pipe) {
					string pipe_error = "Failed to open bash shell when trying to run command\n\r\n";
					send(sock, pipe_error.c_str(), pipe_error.length(), 0);
					warn("%s",pipe_error.c_str());
				}
				
				while (fgets(buffer, 128, pipe) != NULL) {
					result += buffer;
				}
				// printf("%s\n", result.c_str());
				// if((result_int = result.find("command not found")) <  0){
				// 	send(sock, error_command.c_str(), error_command.length(), 1);
				// }
				pclose(pipe);
				// if((result_int = result.find("command not found")) >=  0){
				// printf("here");
				if(result != ""){
					result += "\r\n";
					send(sock, result.c_str(), result.length(), 0);
				}else{
					string error_command = "sh: " + temp.substr(0, temp.length() - 2) + ": Command not Found\n\r\n";
					send(sock, error_command.c_str(), error_command.length(), 0);
				}
				// }

				// printf("okay");
				result = "";
				temp = "";
			}
		}
	}catch(...){
		warn("Internal Server error has occured");
	    string header = "Internal Server Error be restablish connection";
		char *char_header = new char[header.length()];
		strcpy(char_header, header.c_str());
		send(sock, char_header, sizeof(char_header), 0);

	}
	exit(0);
}

int main(int argc, char * argv[]) {
//   int listen_fd = guard(socket(PF_INET, SOCK_STREAM, 0), "Could not create TCP socket");
//   printf("Created new socket %d\n", listen_fd);
//   guard(listen(listen_fd, 100), "Could not listen on TCP socket");
//   struct sockaddr_in listen_addr;
//   socklen_t addr_len = sizeof(listen_addr);
//   guard(getsockname(listen_fd, (struct sockaddr *) &listen_addr, &addr_len), "Could not get socket name");
//   printf("Listening for connections on port %d\n", ntohs(listen_addr.sin_port));

	if(argc != 2){
		warn("Incorrect number of args");
		exit(0);
	}

	struct sockaddr_in servaddr;
	struct sockaddr_storage their_addr;
	socklen_t addr_size;
 
    int main_socket = socket(AF_INET, SOCK_STREAM, 0);

	char* port = argv[1];

	//Create Listen Socket
    bzero( &servaddr, sizeof(servaddr));
 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(atoi(port));
 
    bind(main_socket, (struct sockaddr *) &servaddr, sizeof(servaddr));
	listen (main_socket, 16);
	int new_fd;
	// char c;
	string fork_error = "Could not fork";
	string recv_error = "Could not recv on TCP connection";
	while(main_socket > 0){
		new_fd = accept(main_socket, (struct sockaddr *)&their_addr, &addr_size);
		//parse_recv(new_fd);
		if(new_fd > 0){
			// printf("Got new connection %d\n", new_fd);
			if (guard(fork(), (char *) fork_error.c_str()) == 0) {
				establish_connnection(new_fd);
				close(new_fd);	
			}
		}
	}
	return 0;
}
