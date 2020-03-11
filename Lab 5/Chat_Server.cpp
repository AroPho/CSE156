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
#include <map>
using namespace std;

int out = 0;
int in = 0;
sem_t empty, full;
int buff[4];
string client_names[4];
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_list = PTHREAD_MUTEX_INITIALIZER;

map<string, int> contact_list;
map<string, int> connections;
map<int, int> ports;

void add_to_list(int sock, string name){
    pthread_mutex_lock(&mutex_list);
    contact_list[name] = sock;
    pthread_mutex_unlock(&mutex_list);
    string msg = "wait\r\n\r\n";
    send(sock, msg.c_str(), msg.length(), 0);
}

void remove_from_list(string name){
    pthread_mutex_lock(&mutex_list);
    contact_list.erase(name);
    pthread_mutex_unlock(&mutex_list);
}

void contact_list_send(int sock){
    string temp = "";
    int count = 1;
    for( map<string, int>::iterator iter=contact_list.begin(); iter!=contact_list.end(); ++iter) {  
        temp += to_string(count) + ") " + (*iter).first + "\n";
        count++;
    }
    temp += "\r\n\r\n";  
    send(sock, temp.c_str(), temp.length(), 0);
}

void connect_clients(int sock, string line){
    // int numbytes;
    // char c;
    pthread_mutex_lock(&mutex_list);
    int other_client ;
    // printf("%s\n", line.c_str());
    map<string, int>::iterator iter =  contact_list.find(line);
    if(iter == contact_list.end()){
        other_client = -1;
    }else{
        other_client = iter -> second;
        contact_list.erase(line.substr(0, line.length() - 4));
    }
    pthread_mutex_unlock(&mutex_list);

    if(other_client != -1){
        // printf("here\n");
        string ip = "Ip: ";
        struct sockaddr_in addr;
        socklen_t addr_size = sizeof(addr);
        getpeername(other_client, (struct sockaddr *)&addr, &addr_size);
        ip += inet_ntoa(addr.sin_addr);
        // printf("%s\n", ip.c_str());

        map<int, int>::iterator Piter =  ports.find(other_client);
        int port = Piter -> second;
        
        ip += " " + to_string(port) + "\r\n\r\n";
        // printf("%s\n", ip.c_str());
        send(sock, ip.c_str(), ip.length(), 0);
    }else{
        string temp = "Error: " + line.substr(0, line.length() - 4) + " is no longer waiting for a connection or you typed the name wrong\r\n\r\n";
        send(sock, temp.c_str(), temp.length(), 0);
    }
    
}

string first_contact(int sock){
    int numbytes;
    char c;
    string temp = "";
    string name;
    while((numbytes = recv(sock, &c, 1, 0)) != 0){
        temp += c;
        if(temp.length() >= 4 && temp.substr(temp.length() - 4) == "\r\n\r\n"){
            // contact_list[temp.substr(0, temp.length() - 4)] = sock;
            name = temp.substr(0, temp.length() - 4);
            break;
        }
    }
    map<string, int>::iterator iter = connections.find(name);
    if(iter != connections.end()){
        // printf("here");
        return "NO\r\n\r\n";
    }
    connections[name] = sock;
    printf("%s\n", name.c_str());
    return name;
}

void command_find(string line, string name, int sock){
    printf("%s\n", line.c_str());
    
    if(line != "/wait" && line.substr(0,9) != "/connect " &&  line != "/list" && line != "/quit" && line.substr(0,6) != "Port: " ){
        string error = "Command " +  line + " not recognized\r\n\r\n";
        send(sock, error.c_str(), error.length(), 0);
        return;
    }

    if(line == "/list"){
        // printf("here");
        contact_list_send(sock);
    }
    if(line ==  "/wait"){
        add_to_list(sock, name);
    }
     if(line == "/quit"){
        remove_from_list(name);
    }
    if(line.substr(0,9) == "/connect "){
        connect_clients(sock, line.substr(9));
    }
    if(line.substr(0,6) == "Port: "){
        string port = line.substr(line.find(" ") + 1);
        // printf("%s\n", port.c_str());
        ports[sock] = stoi(port);
    }
}

void *establish_connection(void *){
    int numbytes;
    // int end_header = 0;
    char c;
    string temp = "";
    string name;
    int socket;
    while(1){
		//Consumer code
		sem_wait(&full);
		pthread_mutex_lock(&mutex1);
		name = client_names[out];
		socket = buff[out];
		out = (out + 1) % 4;
		pthread_mutex_unlock(&mutex1);
		sem_post(&empty);

        try{
            while((numbytes = recv(socket, &c, 1, 0)) != 0){
                temp += c;
                if(temp.length() >= 4 && temp.substr(temp.length() - 4) == "\r\n\r\n"){
                    // printf("%s\n", temp.substr(0, temp.length() - 4).c_str());
                    command_find(temp.substr(0, temp.length() - 4), name, socket);
                    temp = "";
                }
            }
            connections.erase(name);
            map<string, int>::iterator iter = contact_list.find(name);
            if( iter != contact_list.end()){
                contact_list.erase(name);
            }
        }catch(...){
            connections.erase(name);
        }
    }
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
	string not_taken = "Good\r\n\r\n";
	string recv_error = "Could not recv on TCP connection";
    string new_name;

    sem_init(&empty, 0, 4);
	sem_init(&full, 0, 0);


	while(main_socket > 0){
		new_fd = accept(main_socket, (struct sockaddr *)&their_addr, &addr_size);
        new_name = first_contact(new_fd);
        if(new_name == "NO\r\n\r\n"){
            string repeat = "TAKEN\r\n\r\n";
            send(new_fd, repeat.c_str(), repeat.length(), 0);
            new_fd = 0;
        }
        send(new_fd, not_taken.c_str(), not_taken.length(), 0);
		//parse_recv(new_fd);
		if(new_fd > 0){
			// printf("Got new connection %d\n", new_fd);
            pthread_t tidsi;
            pthread_create(&tidsi, NULL, establish_connection, NULL);

            sem_wait(&empty);
            pthread_mutex_lock(&mutex1);
            client_names[in] = new_name;
            buff[in] = new_fd;
            in = (in + 1) % 4;
            pthread_mutex_unlock(&mutex1);
            sem_post(&full);	
			
		}
	}
	return 0;
}