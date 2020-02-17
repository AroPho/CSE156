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
//#include <sys/time.h>
#include <poll.h>
#include <signal.h>
using namespace std;
#define MAXLINE 1024

int num_args = 4;
int out = 0;
int in = 0;
int current_index = 0; 
int sockfd;
sem_t empty, full;
struct sockaddr_in *buff;
char* char_buffer[4];
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

void sending_packet(int sock, sockaddr_in client, string msg){
    sendto(sock, msg.c_str(), sizeof(msg), 0, (const struct sockaddr *) &client, sizeof(client));
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
    	sending_packet(socket, client, error);
	}
	if(err == 403){
		string content = "Content-Length: " + to_string(0) + "\r\n\r\n";
    	string error = "HTTP/1.1 403 Forbidden\r\n" + content;
    	sending_packet(socket, client, error);
	}
	if(err == 404){
		string content = "Content-Length: " + to_string(0) + "\r\n\r\n";
    	string error = "HTTP/1.1 404 Not Found\r\n" + content;
    	sending_packet(socket, client, error);
	}
}

//Used for get method and parse through a file and sends to client
void printing(int type, int f, int socket, sockaddr_in client, int beginning, int end){
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
    	header = "HTTP/1.1 200 OK\r\n" + content + temp;
	}
	if(type == 2){ // For HEAD requests
		header = "HTTP/1.1 200 OK\r\n" + content;
	}
    sending_packet(socket, client, header);
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
    	printing(1, f, socket, client, beginning, end); // Calls file to start sending client data
   	}else{
		printing(1, f, socket, client, 0, -1); // Calls file to start sending client data
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
    	printing(2, f, socket, client, 0, -1); // Calls file to start sending client data
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

void *parse_recv(void *){
	int numbytes;
	int end_header = 0;
	int method_type = -1;
	char c;
	string body;
	string temp;
	string filename = "no";
	struct sockaddr_in client;
    socklen_t client_size;
	
	// This is the start of the thread code
	while(1){
		//Consumer code
		sem_wait(&full);
		pthread_mutex_lock(&mutex1);
		client = buff[out];
        temp = char_buffer[out];
		out = (out + 1) % num_args;
		pthread_mutex_unlock(&mutex1);
		sem_post(&empty);
        string request_type = "";

        cout << temp;
		// Start of Consumer consume code
		try{
            client_size = sizeof(client);
			while((numbytes = recvfrom(sockfd, &c, 1, 0, (struct sockaddr *) &client, &client_size)) != 0){ //Goes through first line of header passed in to server
				// printf("%c", c);
				// printf("here");
				if(end_header != 1){
					temp += c;
				}
				if(end_header == 0 && temp.length() > 3 && temp.substr(temp.length() - 4) == "\r\n\r\n"){ //Checks for end of header
					end_header = 1;
				}
				if(end_header == 1 && method_type == -1){ // Parses header for request type
					method_type = get_put_checker(temp);
				}
				if(method_type == 1 && end_header == 1){// GET Method function call
					method_type = -1;
					end_header = 0;
					//printf("%s\n", temp.c_str());
					get_parse(temp, sockfd, client);
					temp = "";
				}
				if(method_type == 2 && end_header == 1){ // HEAD Method function call
					head_parse(temp, sockfd, client);
					temp = "";
					method_type = -1;
					end_header = 0;
				}
				if(method_type == 0 && end_header == 1){ // Checks for bad requests
					error_print(400, sockfd, client);
					end_header = 0;
					method_type = -1;
					temp = "";
				}

				
			}	
		}catch(...){
			string content = "Content-Length: " + to_string(0) + "\r\n\r\n";
		    string header = "HTTP/1.1 500 Created\r\n" + content;
			sending_packet(sockfd, client, header);
		}
	}
}


// Driver code 
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

	sem_init(&empty, 0, num_args);
	sem_init(&full, 0, 0);


	// size_t n = num_args;
	buff = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in *)*(num_args*800));
	
	
	//Following codes defines server and sockets for client to connect to
	struct sockaddr_in servaddr, cliaddr;
 
    int main_socket = socket(AF_INET, SOCK_DGRAM, 0);

    // struct timeval tv;
    // tv.tv_sec = 1;
    // tv.tv_usec = 0;
    // setsockopt(main_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
 
    bzero( &servaddr, sizeof(servaddr));
 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(atoi(port));
 
    bind(main_socket, (struct sockaddr *) &servaddr, sizeof(servaddr));
    sockfd = main_socket;
	
    char buffer[MAXLINE];
    int n;
    int len = sizeof(cliaddr);
    hostent * hostp;
    char* client_addr;
	

	try{
		
		for(int i = 0; i < num_args; i++){
			pthread_t tidsi;
			pthread_create(&tidsi, NULL, parse_recv, NULL);
		}

		//Searches for any connection attempts to server and creates a socket to connect to client
		while(main_socket > 0){
                /*
            * recvfrom: receive a UDP datagram from a client
            */
            bzero(buffer, MAXLINE);
            n = recvfrom(main_socket, buffer, MAXLINE, 0, (struct sockaddr *) &cliaddr, &addr_size);
            if (n < 0){
                warn("ERROR in recvfrom");
            }

            /* 
            * gethostbyaddr: determine who sent the datagram
            */
            hostp = gethostbyaddr((const char *)&cliaddr.sin_addr.s_addr, sizeof(cliaddr.sin_addr.s_addr), AF_INET);
            if (hostp == NULL){
                warn("ERROR on gethostbyaddr");
            }
            client_addr = inet_ntoa(cliaddr.sin_addr);
            if (client_addr == NULL){
                warn("ERROR on inet_ntoa\n");
            }

            // sem_wait(&empty);
            // pthread_mutex_lock(&mutex1);
            // buff[in] = cliaddr;
            // char_buffer[in] = buffer;
            // in = (in + 1) % 4;
            // pthread_mutex_unlock(&mutex1);
            // sem_post(&full);

            printf("server received datagram from %s (%s)\n", hostp->h_name, client_addr);
            
            printf("server received %zu/%d bytes: %s\n", strlen(buffer), n, buffer);

            /* 
            * sendto: echo the input back to the client 
            */
            n = sendto(main_socket, buffer, strlen(buffer), 0, 
                (struct sockaddr *) &cliaddr, len);
            if (n < 0) 
            warn("ERROR in sendto");	
			
		}
		close(main_socket);
		//free(buff);
		return 0;
	}catch(...){
		string content = "Content-Length: " + to_string(0) + "\r\n\r\n";
	    string header = "HTTP/1.1 500 Created\r\n" + content;
		char *char_header = new char[header.length()];
		strcpy(char_header, header.c_str());
		send(main_socket, char_header, sizeof(char_header), 0);
    }




        
}
