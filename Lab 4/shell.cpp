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

        string hostname = argv[1];
        string port = argv[2];
        // printf("%s %s\n", hostname.c_str(), port.c_str());
        
        // cout << port;
        // printf("%s", hostname.c_str());
        int new_fd;
        getaddrinfo(hostname.c_str(), port.c_str(), &hints, &addrs);
        new_fd = socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol);
        int confirm = connect(new_fd, addrs->ai_addr,addrs->ai_addrlen);
        printf("%d\n", confirm);
        int numbytes;
        string input;

        
        // host_buff = (string*) malloc(sizeof(string)*(num_args*800));

        //cout << 2 << "\n";
    
        // exit(1);
    try{
        char c;
        while(1){
            getline(cin, input);
            input += "\n";
            send(new_fd, input.c_str(), input.length(), 0);
            recv(new_fd, &c, 1, 0);
            printf("%c", c);
        }

    }catch(...){
        warn("Warning internal server error closing connections");
        string content = "Content-Length: " + to_string(0) + "\r\n\r\n";
        string header = "HTTP/1.1 500 Created\r\n" + content;
        send(new_fd, header.c_str(), header.length(), 0);
    }
}