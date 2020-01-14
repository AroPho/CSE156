#include <sys/types.h>
#include <sys/socket.h>
#include <bits/stdc++.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <err.h>
#include <fcntl.h>
using namespace std;

// Checks if string contains length of file
int catch_length(string line){
	int temp;
	string temp_string;
	if((temp = line.find("Content")) >= 0){
		temp_string = line.substr(temp);
		int first = (temp_string.find("Content-Length:") + 16);// Used to get filesize
		int last = (temp_string.find("\r\n")) - first;
		string ftemp = temp_string.substr(first,last);
		int size;
		size = stoi(ftemp);
		return size;
	}
	return -1;
}

int main(int argc, char * argv[]){
    if(argc < 3){
        warn("Insufficient number of arguements givin");
    }
    bool head_bool = false;

    char opt;
	while((opt = getopt(argc, argv, "h")) != -1){
		switch(opt){
			case 'h':
				head_bool = true;
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
    string header_send = "HEAD " + file + " HTTP/1.1\r\nHost: " + hostname + "\r\n\r\n";

    struct addrinfo hints, *res;
    int sockfd;

    memset(&hints, 0,sizeof hints);
    hints.ai_family=AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo(hostname, port.c_str(), &hints, &res);
    sockfd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
    connect(sockfd,res->ai_addr,res->ai_addrlen);
    if(head_bool){
        send(sockfd, header_send.c_str(), header_send.length(), 0);
    }
    if(!head_bool){
        send(sockfd, get_request.c_str(), get_request.length(), 0);
    }
    
    int numbytes;
    int written = 0;
    int end_header = 0;
    int length = -1;
    string temp;
    string filename = "output.dat";
    char c;
    remove(filename.c_str());
    int fd = open(filename.c_str(), O_WRONLY | O_CREAT, 0777);
    while((numbytes = recv(sockfd, &c, 1, 0)) != 0){
        temp += c;
        // printf("%c", c);
        if(end_header == 1 || head_bool){
            written += write(fd, &c, 1);
        }
        if(written == length){
            break;
        }
        if(end_header == 0 && temp.length() > 3 && temp.substr(temp.length() - 4) == "\r\n\r\n"){ //Checks for end of header
                end_header = 1;
                if(head_bool){
                    break;
                }
                length = catch_length(temp);
        }
    }
    close(fd);
    close(sockfd);
}
