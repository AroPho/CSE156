#include <sys/types.h>
#include <sys/socket.h>
// #include <bits/stdc++.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <err.h>
#include <fcntl.h>
#include <arpa/inet.h>
using namespace std;

// Checks if string contains length of file
int catch_length(string line){
	int temp;
	string temp_string;
	if((temp = line.find("Content-Length")) >= 0){
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
    
    //Checks for appropriate number of args
    if(argc < 3){
        warn("Insufficient number of arguements givin");
    }
    if(argc > 4){
        warn("To many arguements givin");
    }
    bool head_bool = false;

    // checks if there is a -h present in args
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

    // Getting port/path/hostname from args
    char * hostname = argv[0];
    string port  = "";
    char * path = argv[1];
    string ip;


    // Use to determine path and ip of request
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

    // Creates appropriate GET and HEAD HTTP Request
    string get_request = "GET " + file + " HTTP/1.1\r\nHost: " + hostname + "\r\n\r\n";
    string header_send = "HEAD " + file + " HTTP/1.1\r\nHost: " + hostname + "\r\n\r\n";

    // struct addrinfo hints, *res;
    // int sockfd;

    // memset(&hints, 0,sizeof hints);
    // hints.ai_family=AF_UNSPEC;
    // hints.ai_socktype = SOCK_STREAM;
    // getaddrinfo(hostname, port.c_str(), &hints, &res);
    // sockfd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);

    // Creates and establishes socket connection
    // struct addrinfo hints, *res;
    // int sockfd;

    struct addrinfo hints, *addrs;
	struct sockaddr_storage their_addr;
	socklen_t addr_size;

    memset(&hints, 0,sizeof hints);
    hints.ai_family=AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo(hostname, port.c_str(), &hints, &addrs);
    int sockfd = socket(addrs->ai_family,addrs->ai_socktype,addrs->ai_protocol);
    connect(sockfd,addrs->ai_addr,addrs->ai_addrlen);
       

    
    try{
        // connect(sockfd,res->ai_addr,res->ai_addrlen);
        
        // Checks for what type of http request needs to be sent
        if(head_bool){
            send(sockfd, header_send.c_str(), header_send.length(), 0);
        }
        if(!head_bool){
            send(sockfd, get_request.c_str(), get_request.length(), 0);
        }
        // cout << "here";
        int numbytes;
        int written = 0;
        int end_header = 0;
        int length = -1;
        string temp;
        string filename = "output.dat";
        char c;
        int fd;

        // If GET request create output.dat file
        if(!head_bool){
            remove(filename.c_str());
            fd = open(filename.c_str(), O_WRONLY | O_CREAT, 0777);
        }
        // cout << "here2\n";

        // Starts recieving response from server
        while((numbytes = recv(sockfd, &c, 1, 0)) != 0){
            if(end_header != 1){
              temp += c;
            }

            //Writes to file if end of header
            if(end_header == 1){
                written += write(fd, &c, 1);
            }
            if(head_bool){
                printf("%c", c);
            }
            if(written == length){
                break;
            }

            // Checks for end of header
            if(end_header == 0 && temp.length() > 3 && temp.substr(temp.length() - 4) == "\r\n\r\n"){ //Checks for end of header
                    end_header = 1;
                    if(head_bool){
                        break;
                    }
                    // cout << "1";
                    length = catch_length(temp);
                    // printf("here");
            }
        }
        close(fd);
        close(sockfd);
    }catch(...){
        close(sockfd);
        warn("Warning internal server error closing connections");
    }
}
