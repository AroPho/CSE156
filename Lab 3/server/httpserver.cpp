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

int out = 0;
int in = 0;
int current_index = 0;
int main_socket;
sem_t empty, full;
string buff[4];
struct sockaddr_in client_connections[4];
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;


void sending_packet(int sock, string msg, sockaddr_in client){
	socklen_t clientlen = sizeof client;
	char * client_addr = inet_ntoa(client.sin_addr);
	// printf("client address %s", client_addr);
	// printf("%s", msg.c_str());
    sendto(sock, msg.c_str(), msg.length(), 0, (struct sockaddr *) &client, clientlen);
}

// Catches Range requests 
void catch_range(string line, int *start, int *end){
	int temp;
	string temp_string;
	if((temp = line.find("Content-Range")) >= 0){
		temp_string = line.substr(temp);
		int first = (temp_string.find("Content-Range:") + 15);// Used to get filesize
		string temp2 = temp_string.substr(first);
		int middle = (temp2.find("-") + first);
		int last = ((temp_string.find("/")) - middle) -1 ;
		string ftemp = temp_string.substr(first,last);
		// printf("%d, %d\n", middle - first, last);
		*start = stoi(temp_string.substr(first, middle - first));
		*end = stoi(temp_string.substr(middle + 1, last)); 
	}else{
		*start = -1;
		*end = -1;
	}
}

// prints out http error response codes
void error_print(int err, int socket, sockaddr_in client){
	if(err == 400){
		string content = "Content-Length: " + to_string(0) + "\r\n\r\n";
		string error = "HTTP/1.1 400 Bad Request\r\n" + content;
    	sending_packet(socket, error, client);
	}
	if(err == 403){
		string content = "Content-Length: " + to_string(0) + "\r\n\r\n";
    	string error = "HTTP/1.1 403 Forbidden\r\n" + content;
    	sending_packet(socket, error, client);
	}
	if(err == 404){
		string content = "Content-Length: " + to_string(0) + "\r\n\r\n";
    	string error = "HTTP/1.1 404 Not Found\r\n" + content;
    	sending_packet(socket, error, client);
	}
}

//Used for get method and parse through a file and sends to client
void printing(int type, int f, int socket, int beginning, int end, sockaddr_in client){
    string temp, header;
	int temp_begin = beginning;
    int size, numbytes;
    char c;
    while((numbytes = pread(f, &c, 1, temp_begin)) > 0 && temp_begin != end){
    	// printf("%c", c);
		// printf("%d\n", temp_begin);
		temp += c;
		temp_begin += numbytes;
    }
    size = temp.length();
    string content = "Content-Length: " + to_string(size) + "\r\n\r\n";
	
	if(type == 1){ // For GET requests
		string range = "Content-Range: " + to_string(beginning) + "-" + to_string(end) + "/" + to_string(size) + "\r\n";
    	header = "HTTP/1.1 200 OK\r\n" + range + content + temp;
	}
	if(type == 2){ // For HEAD requests
		header = "HTTP/1.1 200 OK\r\n" + content;
	}
    sending_packet(socket, header, client);
}

//Used to execute Get request
void get_parse(string header, int socket, sockaddr_in client){
	
	int first = (header.find("GET ") + 4);// Used to get Filename
	int last = (header.find("HTTP/1.1")) - first - 1;
	string temp = header.substr(first, last);
	int beginning, end;
	catch_range(header, &beginning, &end);
	
	//Parses filename for path
	if(temp.find("/") == 0){
		temp = temp.substr(1);
	}
	string request_type = "GET";
	if(temp.length() == 28 && temp.at(0) == '/'){// Checks for / at start of filename and ignores
		temp = temp.substr(1, 27);
	}
	//printf("%s\n", temp.c_str());	
	int f = open(temp.c_str(), O_RDONLY);
	if(errno == 13){ // Insufficient Permissions
    	error_print(403, socket, client);
    }
	if(f == -1){
		error_print(404, socket, client);
	}if(beginning != -1){
    	printing(1, f, socket, beginning, end, client); // Calls file to start sending client data
   	}else{
		printing(1, f, socket, 0, -1, client); // Calls file to start sending client data
	}
	   
}

// Used complete HEAD request
void head_parse(string header, int socket, sockaddr_in client){
	int first = (header.find("HEAD ") + 5); // Used to get Filename
	int last = (header.find("HTTP/1.1")) - first - 1;
	string temp = header.substr(first, last);
	
	// Parses filename for paths
	if(temp.find("/") == 0){
		temp = temp.substr(1);
	}
	// printf("%s\n", temp.c_str());	
	int f = open(temp.c_str(), O_RDONLY);
	
	if(errno == 13){ // Server does not have proper permissions
    	error_print(403, socket, client);
    }
	if(f == -1){ // File not Found
		error_print(404, socket, client);
	}else{
    	printing(2, f, socket, 0, -1, client); // Calls file to start sending client data
   	}
}

//This a function that parses each line header to check for method request type
int get_put_checker(string line){
	int temp_typeG = (line.find("GET"));
	int temp_typeH = (line.find("HEAD"));
	if(temp_typeG != -1 && (line.substr(temp_typeG, 3) == "GET")){
		return 1;
	}
	if(temp_typeH != -1 && (line.substr(temp_typeH, 4) == "HEAD")){
		return 2;
	}
	return 0;
}


// This function parses through all the data sent to the server
void *parse_recv(void *){
	// int numbytes;
	// int end_header = 0;
	int method_type = -1;
	// char c;
	string body;
	string temp;
	string filename = "no";
	char *request;
	struct sockaddr_in client;
	
	// This is the start of the thread code
	while(1){
		//Consumer code
		sem_wait(&full);
		pthread_mutex_lock(&mutex1);
		client = client_connections[out];
		temp = buff[out];
		out = (out + 1) % 4;
		pthread_mutex_unlock(&mutex1);
		sem_post(&empty);
        // string request_type = "";
		// printf("%s", temp.c_str());
		
		// Start of Consumer consume code
		try{
			method_type = get_put_checker(temp);
			// printf("here\n");
			// printf("method: %d\n", method_type);	
			if(method_type == 1){// GET Method function call
				method_type = -1;
				//printf("%s\n", temp.c_str());
				get_parse(temp, main_socket, client);
				temp = "";
			}
			if(method_type == 2){ // HEAD Method function call
				// printf("2\n");
				head_parse(temp, main_socket, client);
				method_type = -1;
				temp = "";
			}
			if(method_type == 0){ // Checks for bad requests
				error_print(400, main_socket, client);
				// end_header = 0;
				method_type = -1;
				temp = "";
			}	
				
		}catch(...){
			string content = "Content-Length: " + to_string(0) + "\r\n\r\n";
		    string header = "HTTP/1.1 500 Created\r\n" + content;
			sending_packet(main_socket, header, client);
		}
	}
}

int main(int argc, char * argv[]){
	// struct addrinfo hints, *addrs;
	// struct sockaddr_storage their_addr;
	socklen_t addr_size;
	if(argc < 2){
		warn("%d", argc);
	}	




	//Assigns argv to port and host
	char * port = argv[1];
	// string hostname = "127.0.0.1";

	//printf("%s %s\n",hostname, port);

	sem_init(&empty, 0, 4);
	sem_init(&full, 0, 0);

	char input[1024];


	// size_t n = num_args;
	struct sockaddr_in servaddr, cliaddr;
 
    main_socket = socket(AF_INET, SOCK_DGRAM, 0);

    // struct timeval tv;
    // tv.tv_sec = 1;
    // tv.tv_usec = 0;
    // setsockopt(main_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
 
    bzero( &servaddr, sizeof(servaddr));
 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(atoi(port));
 
    bind(main_socket, (struct sockaddr *) &servaddr, sizeof(servaddr));
	listen (main_socket, 16);
	

	try{
		addr_size = sizeof cliaddr;
		int n;
		
		for(int i = 0; i < 4; i++){
			pthread_t tidsi;
			pthread_create(&tidsi, NULL, parse_recv, NULL);
		}
		string temp = "";

		//Searches for any connection attempts to server and creates a socket to connect to client
		while(main_socket > 0){
			bzero(input, 1024);
			n = recvfrom(main_socket, &input, 1024, 0, (struct sockaddr *)&cliaddr, &addr_size);
			//parse_recv(new_fd);
			temp = input;
			// sendto(main_socket, &input, 1024, 0, (struct sockaddr *)&cliaddr, addr_size);
			if(n > 0){
				//printf("%d\n", new_fd);
				sem_wait(&empty);
				pthread_mutex_lock(&mutex1);
				client_connections[in] = cliaddr;
				buff[in] = temp;
				in = (in + 1) % 4;
				pthread_mutex_unlock(&mutex1);
				sem_post(&full);	
			}
			// printf("%s", input);
		}
		close(main_socket);
		// free(buff);
		return 0;
	}catch(...){
		string content = "Content-Length: " + to_string(0) + "\r\n\r\n";
	    string header = "HTTP/1.1 500 Created\r\n" + content;
		char *char_header = new char[header.length()];
		strcpy(char_header, header.c_str());
		send(main_socket, char_header, sizeof(char_header), 0);
	}
}
