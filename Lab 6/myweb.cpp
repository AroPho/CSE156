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
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>


using namespace std;

// #define CERT_FILE  HOME "1024ccert.pem"
// #define KEY_FILE  HOME  "1024ckey.pem"
// #define CIPHER_LIST "AES128-SHA"
// #define CA_FILE "/certs/1024ccert.pem"
// #define CA_DIR  NULL
// #define KEY_PASSWD "keypass"

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

void ShutdownSSL(SSL *cSSL)
{
    SSL_shutdown(cSSL);
    SSL_free(cSSL);
}

 void log_ssl()
{
    int err;
    while (err = ERR_get_error()) {
        char *str = ERR_error_string(err, 0);
        if (!str)
            return;
        printf("%s\n", str);
        fflush(stdout);
    }
}

void ShowCerts(SSL* ssl)
{   X509 *cert;
    char *line;

    cert = SSL_get_peer_certificate(ssl); /* get the server's certificate */
    if ( cert != NULL )
    {
    printf("Server certificates:\n");
    line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
    printf("Subject: %s\n", line);
    free(line);       /* free the malloc'ed string */
    line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
    printf("Issuer: %s\n", line);
    free(line);       /* free the malloc'ed string */
    X509_free(cert);     /* free the malloc'ed certificate copy */
}
else
    printf("No certificates.\n");
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

void https(int sock, string file, string hostname){
    
    InitializeSSL();
    

    const SSL_METHOD *meth = TLSv1_2_client_method();
    
    
    SSL_CTX *ctx = SSL_CTX_new (meth);
    SSL_CTX_set_options(ctx, SSL_OP_SINGLE_DH_USE);
    // int use_cert = SSL_CTX_use_certificate_file(ctx, "/serverCertificate.pem" , SSL_FILETYPE_PEM);

    // int use_prv = SSL_CTX_use_PrivateKey_file(ctx, "/serverCertificate.pem", SSL_FILETYPE_PEM);


    SSL *ssl_sock = SSL_new(ctx);
    SSL_set_fd(ssl_sock, sock);
    //Here is the SSL Accept portion.  Now all reads and writes must use SSL
    int ssl_err = SSL_connect(ssl_sock);
    ShowCerts(ssl_sock);
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

    if(ssl_err <= 0)
    {
        //Error occurred, log and close down ssl
        printf("Error creating SSL connection.  err=%x\n", ssl_err);
        log_ssl();
        fflush(stdout);
        ShutdownSSL(ssl_sock);
        //printf("error %d\n", ssl_err);
        exit(0);
    }
    try{

        string get_request = "GET " + file + " HTTP/1.1\r\nHost: " + hostname + "\r\n\r\n";
        string header_send = "HEAD " + file + " HTTP/1.1\r\nHost: " + hostname + "\r\n\r\n";
        // connect(sockfd,res->ai_addr,res->ai_addrlen);
        char *head_req = new char [header_send.length() + 1];
        strcpy(head_req, header_send.c_str());
        
        // Checks for what type of http request needs to be sent
        if(head_bool){
            printf("%s\n", head_req);
            int len = SSL_write(ssl_sock, head_req, strlen(head_req));
            if (len < 0) {
                int err = SSL_get_error(ssl_sock, len);
                switch (err) {
                case SSL_ERROR_WANT_WRITE:
                    cout << 0;
                case SSL_ERROR_WANT_READ:
                    cout << 0;
                case SSL_ERROR_ZERO_RETURN:
                case SSL_ERROR_SYSCALL:
                case SSL_ERROR_SSL:
                default:
                    cout << -1;
                }
            }
        }
        if(!head_bool){
            SSL_write(ssl_sock, get_request.c_str(), get_request.length());
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
        while((numbytes = SSL_read(ssl_sock, &c, 1)) != 0){
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
        ShutdownSSL(ssl_sock);
        close(sock);
    }catch(...){
        close(sock);
        ShutdownSSL(ssl_sock);
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
        last = getnthindex(path, ':', 2);
        if(last != -1){

            first = path.find("://") + 3;
            hostname_str = path.substr(first, last - first); 
            path = path.substr(last);
            last = path.find("/");
            first = path.find(":");
            file = path.substr(last);
            port = path.substr(first + 1, last - first - 1);

        }else{

            port = "443";
            last = getnthindex(path, '/', 3);
            first = path.find("://") + 3;
            file = path.substr(last);
            hostname_str = path.substr(first, last - first);

        }
    }else{
        if((first = path.find(":") ) != -1){

            hostname_str = path.substr(0, first);
            last = path.find("/");
            port = path.substr(path.find(":") + 1, last - first - 1 );

        }else{

            hostname_str = path.substr(0, path.find("/"));
            port = "443";
        
        }
        file = path.substr(path.find("/"));
    }
    printf("%s %s\n", hostname_str.c_str(), port.c_str());
    path = hostname;


    

    // Creates appropriate GET and HEAD HTTP Request


    struct addrinfo hints, *addrs;
	// struct sockaddr_storage their_addr;
	// socklen_t addr_size;

    memset(&hints, 0,sizeof hints);
    hints.ai_family=AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo(hostname_str.c_str(), port.c_str(), &hints, &addrs);
    int sockfd = socket(addrs->ai_family,addrs->ai_socktype,addrs->ai_protocol);

    connect(sockfd,addrs->ai_addr,addrs->ai_addrlen);

    // struct sockaddr_in addr;
    // socklen_t addr_size = sizeof(addr);
    // getpeername(sockfd, (struct sockaddr *)&addr, &addr_size);
    // string ip = inet_ntoa(addr.sin_addr);
    // printf("%s\n", ip.c_str());

    if(path.substr(0,5) == "https"){
        // printf("here\n");
        https(sockfd, file, hostname_str);
    }else{
        no_https(sockfd, file, hostname_str);
    }
}
