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
#include <fstream>
#include <poll.h>
#include <signal.h>
#include <fstream>
using namespace std;

bool connection_bool = false;
string client_name;

int guard(int n, char * err) { if (n == -1) { perror(err); exit(1); } return n; }


void *p2p_send(void *args){
    // struct sockaddr_in cliaddr;
    // socklen_t addr_size;
    int sock = *((int*)args);
    //char input[1024];
    // int n;
    string input;
    string sending = "";
    // bzero(input, 1024);
    while(connection_bool){
        printf("%s> ", client_name.c_str());
        sending = client_name + ": ";
        getline(cin, input);
        input += "\r\n";
        sending += input;
        send(sock, input.c_str(), input.length(), 0);
    }
    return NULL;


}

void *p2p_recieve(void *args){
    char input[1024];
    int n;
    int sock = *((int*)args);
    string temp = "";
    while((n = recv(sock, &input, 1024,0)) != 0){
        temp += input;
        if(temp.length() > 2 && temp.substr(temp.length() -2) == "\r\n"){
            printf("\n%s\n%s>  ", temp.substr(0, temp.length() -2).c_str(), client_name.c_str());
        }
        bzero(input, 1024);
        
    }
    connection_bool = false;
    return NULL;

}

void p2p_connect_connect(string command){
    struct sockaddr_in servaddr;
    string line = command.substr(5);

    string hostname = line.substr(0, line.find(" "));
    string port = line.substr((line.find(" ") + 1));

    int int_arr[1];

    bzero(&servaddr, sizeof(servaddr)); 
    servaddr.sin_addr.s_addr = inet_addr(hostname.c_str()); 
    servaddr.sin_port = htons(stoi(port)); 
    servaddr.sin_family = AF_INET; 
    
    // create datagram socket 
    int new_fd = socket(AF_INET, SOCK_DGRAM, 0);

    int does_it_work;
    
    if((does_it_work = connect(new_fd,(struct sockaddr *)&servaddr, sizeof(servaddr))) == -1){
        new_fd = 0;
    }
    if(new_fd != 0){
        string ping = "ping_client\r\n\r\n";
        send(new_fd, ping.c_str(), ping.length(), 0);
        // string fork_error = "Could not fork";
        
        int_arr[0] = new_fd;
        connection_bool = true;
        pthread_t tids1;
		pthread_create(&tids1, NULL, p2p_recieve, (void*)(int_arr + new_fd));
        pthread_t tids2;
		pthread_create(&tids2, NULL, p2p_send, (void*)(int_arr + new_fd));
        // establish_connnection(new_fd);
        
    }
}

void p2p_wait_connect(int sock){
    struct sockaddr_in servaddr, cliaddr;
    // struct sockaddr_storage their_addr;

    int main_socket = socket(AF_INET, SOCK_DGRAM, 0);
    int int_arr[1];

    //Create Listen Socket
    bzero( &servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(0);

    bind(main_socket, (struct sockaddr *) &servaddr, sizeof(servaddr));
    listen (main_socket, 16);

    socklen_t len = sizeof(servaddr);
    getsockname(main_socket, (struct sockaddr *) &servaddr, &len);
    string port = to_string(ntohs(servaddr.sin_port)) + "\r\n\r\n";
    send(sock, port.c_str(), port.length(), 0);

    char input[1024];
    socklen_t addr_size = sizeof cliaddr;
    // string fork_error = "Could not fork";
    int n;
    n = recvfrom(main_socket, &input, 1024, 0, (struct sockaddr *)&cliaddr, &addr_size);
    if(n > 0){
        int new_fd = socket(AF_INET, SOCK_DGRAM, 0);
        connect(new_fd, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
        int_arr[0] = new_fd;
        connection_bool = true;
        pthread_t tids1;
		pthread_create(&tids1, NULL, p2p_recieve, (void*)(int_arr + new_fd));
        pthread_t tids2;
		pthread_create(&tids2, NULL, p2p_send, (void*)(int_arr + new_fd));
        // if (guard(fork(), (char *) fork_error.c_str()) == 0) {
        // p2p_communicate(new_fd);
        // // establish_connnection(new_fd);
        // exit(0);
        // }
    }		
	close(main_socket);
}

// Recieves data from Server
void recieving(int socket){
    int numbytes;
    char c;
    string temp = "";
    while(1){
        while((numbytes = recv(socket, &c, 1, 0)) != 0){
            temp += c;
            if(temp.length() > 3 && temp.substr(temp.length() - 4) == "\r\n\r\n"){ //Checks for end of header
                break;
            }
        }
        if(numbytes == 0){
            printf("Connection to server has been closed");
            exit(0);
        }
        if(temp == "ping\r\n\r\n"){
            // printf("Connection from %s", temp.substr(temp.find("Name: ") + 6, temp.length() - temp.find("Name: ") + 6).c_str());
            // int first = temp.find(" ") + 1;
            // int last = temp.length() - first - 1;
            p2p_wait_connect(socket);
        }
        if(temp.substr(0, 4) == "Ip: "){
            p2p_connect_connect(temp.substr(0, temp.length() - 4));

        }else{
            printf("%s\n", temp.substr(0,temp.length() - 4).c_str());
        }
        if(numbytes == 0){
            printf("Connection to Server has closed, Shuting Down\n");
            exit(0);
        }
    }
}

void first_contact(int sock){
    string temp = client_name + "\r\n\r\n";
    send(sock, temp.c_str(), temp.length(), 0);
}


int main(int argc, char * argv[]){
    //Checks for appropriate number of args

        if(argc != 4){
            warn("Incorrect number of arguements");
            exit(0);
        }

        struct addrinfo hints, *addrs;
        memset(&hints, 0,sizeof hints);
        hints.ai_family=AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        string hostname = argv[1];
        string port = argv[2];
        client_name = argv[3];
        // printf("%s %s\n", hostname.c_str(), port.c_str());
        
        // cout << port;
        // printf("%s", hostname.c_str());
        int new_fd;
        getaddrinfo(hostname.c_str(), port.c_str(), &hints, &addrs);
        new_fd = socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol);
        int confirm = connect(new_fd, addrs->ai_addr,addrs->ai_addrlen);
        if(confirm == -1){
            warn("Unable to connect to server. Please try again or try different IP");
            exit(0);
        }
        
        first_contact(new_fd);
        

    try{
        // char c;
        string input;
        while(1){
            // Gets input from stdin
            printf("%s> ", client_name.c_str());
            getline(cin, input);

            // Prevents terminal texts from being used
            input += "\r\n\r\n";
            send(new_fd, input.c_str(), input.length(), 0);
            recieving(new_fd);       
        }

    }catch(...){
        warn("Warning internal server error closing connections");
        string content = "Content-Length: " + to_string(0) + "\r\n\r\n";
        string header = "HTTP/1.1 500 Created\r\n" + content;
        send(new_fd, header.c_str(), header.length(), 0);
    }
}