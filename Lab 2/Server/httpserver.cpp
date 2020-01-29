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
using namespace std;

int num_args = 4;
int out = 0;
int in = 0;
int current_index = 0; 
sem_t empty, full;
int *buff;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

int catch_range(string line){
	int temp;
	string temp_string;
	if((temp = line.find("Content-Range")) >= 0){
		temp_string = line.substr(temp);
		int first = (temp_string.find("Content-Range") + 15);// Used to get filesize
		int last = (temp_string.find("\r\n")) - first;
		string ftemp = temp_string.substr(first,last);
		int size;
		size = stoi(ftemp);
		return size;
	}
	return -1;
}

// prints out http error response codes
void error_print(int err, int socket){
	if(err == 400){
		string content = "Content-Length: " + to_string(0) + "\r\n\r\n";
		string error = "HTTP/1.1 400 Bad Request\r\n" + content;
    	send(socket, error.c_str(), error.length(), 0);
	}
	if(err == 403){
		string content = "Content-Length: " + to_string(0) + "\r\n\r\n";
    	string error = "HTTP/1.1 403 Forbidden\r\n" + content;
    	send(socket, error.c_str(), error.length(), 0);
	}
	if(err == 404){
		string content = "Content-Length: " + to_string(0) + "\r\n\r\n";
    	string error = "HTTP/1.1 404 Not Found\r\n" + content;
    	send(socket, error.c_str(), error.length(), 0);
	}
}

//Used for get method and parse through a file and sends to client
void printing(int type, int f, int socket ){
    string temp, header;
    int size;
    char c;
    while(read(f, &c, 1) > 0){
    	temp += c;
    }
    size = temp.length();
    string content = "Content-Length: " + to_string(size) + "\r\n\r\n";
	if(type == 1){
    	header = "HTTP/1.1 200 OK\r\n" + content + temp;
	}
	if(type == 2){
		header = "HTTP/1.1 200 OK\r\n" + content;
	}
    send(socket, header.c_str(), header.length(), 0);
}

//Used to execute Get request
void get_parse(string header, int socket){
	int first = (header.find("GET ") + 4);// Used to get Filename
	int last = (header.find("HTTP/1.1")) - first - 1;
	string temp = header.substr(first, last);
	if(temp.find("/") == 0){
		temp = temp.substr(1);
	}
	string request_type = "GET";
	if(temp.length() == 28 && temp.at(0) == '/'){// Checks for / at start of filename and ignores
		temp = temp.substr(1, 27);
	}
	//printf("%s\n", temp.c_str());	
	int f = open(temp.c_str(), O_RDONLY);
	if(errno == 13){
    	error_print(403, socket);
    }
	if(f == -1){
		error_print(404, socket);
	}else{
    	printing(1, f, socket); // Calls file to start sending client data
   	}
}

void head_parse(string header, int socket){
	int first = (header.find("HEAD ") + 5);
	int last = (header.find("HTTP/1.1")) - first - 1;
	string temp = header.substr(first, last);
	if(temp.find("/") == 0){
		temp = temp.substr(1);
	}
	// printf("%s\n", temp.c_str());	
	int f = open(temp.c_str(), O_RDONLY);
	if(errno == 13){
    	error_print(403, socket);
    }
	if(f == -1){
		error_print(404, socket);
	}else{
    	printing(2, f, socket); // Calls file to start sending client data
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
	int numbytes;
	int end_header = 0;
	int method_type = -1;
	char c;
	string body;
	string temp;
	string filename = "no";
	// int fd;
	// int length = -1;
	// int written = 0;
	int socket;
	// int put_phase = 0;
	//int patch_phase = 0;
	
	// This is the start of the thread code
	while(1){
		//Consumer code
		sem_wait(&full);
		pthread_mutex_lock(&mutex1);
		socket = buff[out];
		out = (out + 1) % num_args;
		pthread_mutex_unlock(&mutex1);
		sem_post(&empty);
        string request_type = "";

		// Start of Consumer consume code
		try{
			while((numbytes = recv(socket, &c, 1, 0)) != 0){ //Goes through first line of header passed in to server
				// printf("%c", c);
				if(end_header != 1){
					temp += c;
				}
				if(end_header == 0 && temp.length() > 3 && temp.substr(temp.length() - 4) == "\r\n\r\n"){ //Checks for end of header
					end_header = 1;
				}
				if(end_header == 1 && method_type == -1){
					method_type = get_put_checker(temp);
				}
				if(method_type == 1 && end_header == 1){// GET Method function call
					method_type = -1;
					end_header = 0;
					//printf("%s\n", temp.c_str());
					get_parse(temp, socket);
					temp = "";
				}
				if(method_type == 2 && end_header == 1){
					head_parse(temp, socket);
					temp = "";
					method_type = -1;
					end_header = 0;
				}
				if(method_type == 0 && end_header == 1){
					error_print(400, socket);
					end_header = 0;
					method_type = -1;
					temp = "";
				}

				
			}	
		}catch(...){
			string content = "Content-Length: " + to_string(0) + "\r\n\r\n";
		    string header = "HTTP/1.1 500 Created\r\n" + content;
			send(socket, header.c_str(), header.length(), 0);
		}
	}
}

int main(int argc, char * argv[]){
	struct addrinfo hints, *addrs;
	struct sockaddr_storage their_addr;
	socklen_t addr_size;
	if(argc < 2){
		warn("%d", argc);
	}	




	//Assigns argv to port and host
	char * port = argv[1];
	string hostname = "127.0.0.1";

	//printf("%s %s\n",hostname, port);

	sem_init(&empty, 0, num_args);
	sem_init(&full, 0, 0);


	// size_t n = num_args;
	buff = (int*) malloc(sizeof(int)*(num_args*800));
	
	
	//Following codes defines server and sockets for client to connect to
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(hostname.c_str(), port, &hints, &addrs);
	int main_socket = socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol);
	int enable = 1;

	setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
	bind(main_socket, addrs->ai_addr, addrs->ai_addrlen);
	listen (main_socket, 16);
	

	try{
		addr_size = sizeof their_addr;
		int new_fd = 0;
		
		for(int i = 0; i < num_args; i++){
			pthread_t tidsi;
			pthread_create(&tidsi, NULL, parse_recv, NULL);
		}
		// for(int i = 0; i < num_args; i++){
		// 	pthread_join(tids[i], NULL);
		// }

		//Searches for any connection attempts to server and creates a socket to connect to client
		while(main_socket > 0){
			new_fd = accept(main_socket, (struct sockaddr *)&their_addr, &addr_size);
			//parse_recv(new_fd);
			if(new_fd > 0){
				//printf("%d\n", new_fd);
				sem_wait(&empty);
				pthread_mutex_lock(&mutex1);
				buff[in] = new_fd;
				in = (in + 1) % num_args;
				pthread_mutex_unlock(&mutex1);
				sem_post(&full);	
			}
		}
		close(new_fd);
		free(buff);
		return 0;
	}catch(...){
		string content = "Content-Length: " + to_string(0) + "\r\n\r\n";
	    string header = "HTTP/1.1 500 Created\r\n" + content;
		char *char_header = new char[header.length()];
		strcpy(char_header, header.c_str());
		send(main_socket, char_header, sizeof(char_header), 0);
	}
}
