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
    string port  = "8080";
    
    struct addrinfo hints, *res;
    int sockfd;

    memset(&hints, 0,sizeof hints);
    hints.ai_family=AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo(hostname,port.c_str(), &hints, &res);
    sockfd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
    connect(sockfd,res->ai_addr,res->ai_addrlen);
    //string  msg = "Hello";
    int count = 1;
    int n;
    char c[1];
    //char * get_request = "";
    while((n = read(0, c, 1)) > 0){
      if(count == 1){
        send(sockfd, c, 1, 0);
        count--;
      }
    }
}
