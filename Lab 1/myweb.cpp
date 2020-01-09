#include <sys/types.h>
#include <sys/socket.h>
#include <bits/stdc++.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
using namespace std;


int main(int argc, char * argv[]){
    char * hostname = argv[1];
    string port  = "80";
    char * path = argv[2];

    string str_path(path);
    string file = str_path.substr(str_path.find("/"));


    struct addrinfo hints, *res;
    int sockfd;

    memset(&hints, 0,sizeof hints);
    hints.ai_family=AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo(hostname, port.c_str(), &hints, &res);
    sockfd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
    connect(sockfd,res->ai_addr,res->ai_addrlen);
    //string  msg = "Hello";
    // int count = 1;
    int numbytes;
    
    char c[1];
    //char * get_request = "";
    while((numbytes = recv(sockfd, &c, 1, 0)) != 0){
       send(sockfd, c, 1, 0);
    }
}
