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
#include <iostream>
using namespace std;

void recieving(int socket){
    int numbytes;
    char c;
    string temp = "";
    while((numbytes = recv(socket, &c, 1, 0)) != 0){
        temp += c;
        if(temp.length() > 3 && temp.substr(temp.length() - 2) == "\r\n"){ //Checks for end of header
            break;
        }
    }
    if(numbytes == 0){
        printf("Connection to server has been closed");
        exit(0);
    }
    printf("%s\n", temp.substr(0, temp.length() -2 ).c_str());
}


int main(int argc, char * argv[]){
    //Checks for appropriate number of args

        if(argc != 3){
            warn("Incorrect number of arguements");
            exit(0);
        }

        struct addrinfo hints, *addrs;
        memset(&hints, 0,sizeof hints);
        hints.ai_family=AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        string hostname = argv[1];
        string port = argv[2];
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
        //int numbytes;
        string input;

        
        // host_buff = (string*) malloc(sizeof(string)*(num_args*800));

        //cout << 2 << "\n";
    
        // exit(1);
    try{
        // char c;
        while(1){
            printf("client $ ");
            getline(cin, input);
            if(input.substr(0,4) == "vim " || input.substr(0,3) == "vi " || input.substr(0, 5) == "nano " || (input.length() ==  3 && input.substr(0,3) == "vim")|| (input.length() ==  2 && input.substr(0,2) == "vi")|| (input.length() ==  4 && input.substr(0,4) == "nano")){
                warn("%s is not supported in this program", input.c_str());
            }if(input.length() == 4 && input.substr(0,4) == "exit"){
                    printf("Closing connection with server");
                    close(new_fd);
                    exit(0);
            }if((input.length() == 4 || input.length() == 5) && input.substr(0,4) == "echo"){
                printf("\n\n");
            }else{
                input += "\r\n";
                send(new_fd, input.c_str(), input.length(), 0);
                recieving(new_fd);
                
            }
        }

    }catch(...){
        warn("Warning internal server error closing connections");
        string content = "Content-Length: " + to_string(0) + "\r\n\r\n";
        string header = "HTTP/1.1 500 Created\r\n" + content;
        send(new_fd, header.c_str(), header.length(), 0);
    }
}