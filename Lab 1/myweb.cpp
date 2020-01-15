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
#include <arpa/inet.h>
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
    if(argc > 4){
        warn("To many arguements givin");
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
    string port  = "";
    char * path = argv[1];
    string ip;

    string str_path(path);
    string file = str_path.substr(str_path.find("/"));
    int first;
    if((first = str_path.find(":") ) != -1){
        ip = str_path.substr(0, first);
        int last = str_path.find("/");
        port = str_path.substr(str_path.find(":") + 1, last - first - 1 );
    }else{
        ip = str_path.substr(0, str_path.find("/"));
        port = "80";
    
    }

    string get_request = "GET " + file + " HTTP/1.1\r\nHost: " + hostname + "\r\n\r\n";
    string header_send = "HEAD " + file + " HTTP/1.1\r\nHost: " + hostname + "\r\n\r\n";

    struct addrinfo hints, *addrs;
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    int sockfd;

    memset(&hints, 0,sizeof hints);
    hints.ai_family=AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo(hostname, port.c_str(), &hints, &addrs);
    sockfd = socket(addrs->ai_family,addrs->ai_socktype,addrs->ai_protocol);

    // struct sockaddr_in servaddr, srcaddr;
    // int sockfd;
 
    // sockfd=socket(AF_INET,SOCK_STREAM,0);
    // // bzero(&servaddr,sizeof servaddr);


 
    // servaddr.sin_family=AF_INET;
    // servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    // servaddr.sin_port=htons(stoi(port));
 
    // inet_pton(AF_INET,ip.c_str(),&(servaddr.sin_addr));

    // memset(&srcaddr, 0, sizeof(srcaddr));
    // srcaddr.sin_family = AF_INET;
    // srcaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    // srcaddr.sin_port = htons(stoi(port));


    if(/*bind(sockfd, (struct sockaddr*) &srcaddr, sizeof(srcaddr)) == 0*/ bind(sockfd, addrs->ai_addr, addrs->ai_addrlen) == 0){
        printf("it works");
    }else{
        cout << "Fuck";
        printf("Error code: %d\n", errno);
    }

    //connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr));
    listen (sockfd, 16);
    addr_size = sizeof their_addr;
    int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);

       

    
    try{
        // connect(sockfd,res->ai_addr,res->ai_addrlen);
        if(head_bool){
            send(new_fd, header_send.c_str(), header_send.length(), 0);
        }
        if(!head_bool){
            send(new_fd, get_request.c_str(), get_request.length(), 0);
        }
        
        int numbytes;
        int written = 0;
        int end_header = 0;
        int length = -1;
        string temp;
        string filename = "output.dat";
        char c;
        int fd;
        if(!head_bool){
            remove(filename.c_str());
            fd = open(filename.c_str(), O_WRONLY | O_CREAT, 0777);
        }
        while((numbytes = recv(new_fd, &c, 1, 0)) != 0){
            temp += c;
            // printf("%c", c);
            if(end_header == 1){
                written += write(fd, &c, 1);
            }
            if(head_bool){
                printf("%c", c);
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
    }catch(...){
        close(sockfd);
        warn("Warning internal server error closing connections");
    }
}
