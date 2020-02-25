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


int main(int argc, char * argv[]){
    //Checks for appropriate number of args
        struct addrinfo hints, *addrs;
        memset(&hints, 0,sizeof hints);
        hints.ai_family=AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        char *hostname = argv[1];
        char *port = argv[2];
        printf("%s %s\n", hostname, port);
        
        // cout << port;
        // printf("%s", hostname.c_str());

        getaddrinfo(hostname, port, &hints, &addrs);
        int new_fd = socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol);
        int numbytes;

        
        // host_buff = (string*) malloc(sizeof(string)*(num_args*800));

        //cout << 2 << "\n";
    
        // exit(1);
    try{
        char c;
        while((numbytes = recv(new_fd, &c, 1, 0)) != 0){
            cout << c;
            read(0, &c, 1);
            send(new_fd, &c, 1, 0);
        }

    }catch(...){
        warn("Warning internal server error closing connections");
        string content = "Content-Length: " + to_string(0) + "\r\n\r\n";
        string header = "HTTP/1.1 500 Created\r\n" + content;
        send(new_fd, header.c_str(), header.length(), 0);
    }
}