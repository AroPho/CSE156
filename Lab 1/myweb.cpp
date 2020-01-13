#include <sys/types.h>
#include <sys/socket.h>
#include <bits/stdc++.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <err.h>
using namespace std;


int main(int argc, char * argv[]){
    if(argc < 3){
        warn("Insufficient number of arguements givin");
    }
    bool header;

    char opt;
	while((opt = getopt(argc, argv, "N:l:a:")) != -1){
		switch(opt){
			case 'h':
				header = true;
				break;
			case '?':
				break;
		}
	}
    argc -= optind;
	argv += optind;

    char * hostname = argv[0];
    string port  = "80";
    char * path = argv[1];

    string str_path(path);
    string file = str_path.substr(str_path.find("/"));
    int first;
    if((first = str_path.find(":") ) != -1){
        int last = str_path.find("/");
        port = str_path.substr(str_path.find(":") + 1, last - first - 1 );
    }

    string get_request = "GET " + file + " HTTP/1.1\r\nHost: " + hostname + "\r\n\r\n";
    
    struct addrinfo hints, *res;
    int sockfd;

    memset(&hints, 0,sizeof hints);
    hints.ai_family=AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo(hostname, port.c_str(), &hints, &res);
    sockfd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
    connect(sockfd,res->ai_addr,res->ai_addrlen);
    send(sockfd, get_request.c_str(), get_request.length(), 0);
    //string  msg = "Hello";
    // int count = 1;
    int numbytes;
    int written;
    int end_header = 0;
    string temp;
    string filename = "output.dat"
    char c;
    char buff[1];
    int fd = open(filename.c_str(), O_WRONLY | O_CREAT, 0777);
    //char * get_request = "";
    while((numbytes = recv(sockfd, &c, 1, 0)) != 0){
        temp += c;
        if(end_header == 0 && temp.length() > 3 && temp.substr(temp.length() - 4) == "\r\n\r\n"){ //Checks for end of header
                end_header = 1;
        }
        if(end_header == 1 || header){
            written += write(fd, &c, 1);
        }
        printf("%c", c);
    }
    while(1){
       read(0, &buff, 1);
       send(sockfd, buff, 1, 0);
    }
}
