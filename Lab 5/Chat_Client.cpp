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
bool quit = false;

void wait_recieve(int sock){
    char c;
    int numbytes;
    string temp;
    printf("here\n");
    while((numbytes = recv(sock, &c, 1, MSG_DONTWAIT)) != 0 && quit == false){
        temp += c;
        // printf("%d\n", quit);
        if(temp.length() > 3 && temp.substr(temp.length() - 4) == "\r\n\r\n"){ //Checks for end of header
            break;
        }
    }
    if(temp == "ping\r\n\r\n"){
        // p2p_wait_connect(socket);
    }
    if(quit == true){
        printf("2\n");
        string quitting = "/quit\r\n\r\n";
        send(sock, quitting.c_str(), quitting.length(), 0);
    }

}

void *wait(void *){
    string input;
    while(1){
        printf("%s> ", client_name.c_str());
        getline(cin, input);
        if(input == "/quit"){
            printf("1\n");
            quit = true;
            return NULL;
        }
        if(input != "/quit"){
            printf("%s not supported in wait mode\n", input.c_str()); 
        }
        if(connection_bool == true){
            return NULL;
        }
    }
    
}


// Recieves data from Server
void recieving(int socket){
    int numbytes;
    char c;
    string temp = "";
    
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

    // Info -> Chat
    if(temp.substr(0, 4) == "Ip: "){
        // p2p_connect_connect(temp.substr(0, temp.length() - 4));

    }

    //Info -> Wait
    if(temp == "wait\r\n\r\n"){
        pthread_t tidsc;
		pthread_create(&tidsc, NULL, wait, NULL);
        // printf("here");
        wait_recieve(socket);
    }

    // Info -> Info
    if(temp != "wait\r\n\r\n" && temp.substr(0, 4) != "Ip: "){
        printf("%s\n", temp.substr(0,temp.length() - 4).c_str());
    }
    
}

void first_contact(int sock){
    string temp = client_name + "\r\n\r\n";
    send(sock, temp.c_str(), temp.length(), 0);
    int numbytes;
    char c;
    temp = "";
    while((numbytes = recv(sock,&c, 1, 0) != 0)){
        temp += c;
        if(temp.length() >= 4 && temp.substr(temp.length() - 4) == "\r\n\r\n"){
            // printf("%s\n", temp.c_str());
            if(temp == "TAKEN\r\n\r\n"){
                printf("Client ID is already taken");
                exit(0);
            }
            break;
        }
    }
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