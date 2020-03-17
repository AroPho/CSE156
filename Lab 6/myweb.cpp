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
#include <iostream>
#include <fstream>

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

using namespace std;

// #define CERT_FILE  HOME "1024ccert.pem"
// #define KEY_FILE  HOME  "1024ckey.pem"
// #define CIPHER_LIST "AES128-SHA"
// #define CA_FILE "/certs/1024ccert.pem"
// #define CA_DIR  NULL
// #define KEY_PASSWD "keypass"

SSL_CTX *sslctx;
SSL *cSSL;
bool head_bool = false;


void InitializeSSL()
{
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
}

void DestroySSL()
{
    ERR_free_strings();
    EVP_cleanup();
}

void ShutdownSSL()
{
    SSL_shutdown(cSSL);
    SSL_free(cSSL);
}



int getnthindex(string s, char t, int n)
{
    int count = 0;
    for (int i = 0; (unsigned long) i < s.length(); i++)
    {
        if (s[i] == t)
        {
            count++;
            if (count == n)
            {
                return i;
            }
        }
    }
    return -1;
}

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

void ssl_communication(int sock, string file, string hostname){
    sslctx = SSL_CTX_new( TLSv1_2_client_method());
    SSL_CTX_set_options(sslctx, SSL_OP_SINGLE_DH_USE);

    cSSL = SSL_new(sslctx);
    SSL_set_fd(cSSL, sock);
    //Here is the SSL Accept portion.  Now all reads and writes must use SSL
    int ssl_err = SSL_connect(cSSL);
    if(ssl_err <= 0)
    {
        //Error occurred, log and close down ssl
        ShutdownSSL();
    }
    try{

        string get_request = "GET " + file + " HTTP/1.1\r\nHost: " + hostname + "\r\n\r\n";
        string header_send = "HEAD " + file + " HTTP/1.1\r\nHost: " + hostname + "\r\n\r\n";
        // connect(sockfd,res->ai_addr,res->ai_addrlen);
        
        // Checks for what type of http request needs to be sent
        if(head_bool){
            SSL_write(cSSL, header_send.c_str(), header_send.length());
        }
        if(!head_bool){
            SSL_write(cSSL, get_request.c_str(), get_request.length());
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
        while((numbytes = SSL_read(cSSL, &c, 1)) != 0){
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
        ShutdownSSL();
        close(sock);
    }catch(...){
        close(sock);
        warn("Warning internal server error closing connections");
    }
}

void no_https(int sockfd, string file, string hostname){

    string get_request = "GET " + file + " HTTP/1.1\r\nHost: " + hostname + "\r\n\r\n";
    string header_send = "HEAD " + file + " HTTP/1.1\r\nHost: " + hostname + "\r\n\r\n";

    try{
        // connect(sockfd,res->ai_addr,res->ai_addrlen);
        
        // Checks for what type of http request needs to be sent
        if(head_bool){
            printf("%s\n", header_send.c_str());
            send(sockfd, header_send.c_str(), header_send.length(), 0);
        }
        if(!head_bool){
            printf("%s\n", get_request.c_str());
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



int main(int argc, char * argv[]){
    
    //Checks for appropriate number of args
    if(argc < 3){
        printf("ree\n");
    }
    // bool head_bool = false;

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
    string port;
    string path(hostname);
    int first;
    int last;
    string file;
    if((last = getnthindex(path, '/', 3)) == -1 && path.substr(0,4) == "http"){
        // printf("here");
        path += "/";
    }else if((last = path.find("/")) == -1){
        path += "/";
    }

    string hostname_str = "";
    if(path.substr(0,4) == "http"){
        first = getnthindex(path, ':', 2);
        if(first != -1){

            hostname_str = path.substr(0, first); 
            path = path.substr(first);
            last = path.find("/");
            first = path.find(":");
            file = path.substr(path.find("/"));
            port = path.substr(first + 1, last - first - 1);

        }else{

            port = "80";
            last = getnthindex(path, '/', 3);
            first = path.find("://") + 3;
            file = path.substr(first);
            hostname_str = path.substr(first, last - first);

        }
    }else{
        if((first = path.find(":") ) != -1){

            hostname_str = path.substr(0, first);
            last = path.find("/");
            port = path.substr(path.find(":") + 1, last - first - 1 );

        }else{

            hostname_str = path.substr(0, path.find("/"));
            port = "80";
        
        }
        file = path.substr(path.find("/"));
    }
    printf("%s %s\n", hostname_str.c_str(), port.c_str());


    

    // Creates appropriate GET and HEAD HTTP Request



    InitializeSSL();

    struct addrinfo hints, *addrs;
	// struct sockaddr_storage their_addr;
	// socklen_t addr_size;

    memset(&hints, 0,sizeof hints);
    hints.ai_family=AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo(hostname_str.c_str(), port.c_str(), &hints, &addrs);
    int sockfd = socket(addrs->ai_family,addrs->ai_socktype,addrs->ai_protocol);

    connect(sockfd,addrs->ai_addr,addrs->ai_addrlen);

    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(addr);
    getpeername(sockfd, (struct sockaddr *)&addr, &addr_size);
    string ip = inet_ntoa(addr.sin_addr);
    printf("%s\n", ip.c_str());

    if(hostname_str.substr(0,5) == "https"){
        ssl_communication(sockfd, file, hostname_str);
    }else{
        no_https(sockfd, file, hostname_str);
    }
}
