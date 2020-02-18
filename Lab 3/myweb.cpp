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

int num_args = 4;
int out = 0;
int in = 0;
int offset = 0;
int length = -1;
sem_t empty, full;
int * buff;
sockaddr *addr_buff;
// string * host_buff;
int written = 0;
int size_of_chunks;
bool first_connect = false;
string filename;
//int patch_offset= 0;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_write = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_offest = PTHREAD_MUTEX_INITIALIZER;


void sending_packet(int sock, string msg){
    sendto(sock, msg.c_str(), msg.length(), 0, (const struct sockaddr *) NULL, 0);
}

string recieve_packets(int sock){
    int numbytes;
    string temp = "";
    char buffin[1024];
    while((numbytes = recvfrom(sock, &buffin, 1024, 0, (struct sockaddr *)NULL, NULL)) > 0){
        temp += buffin;
        bzero(buffin, 1024);
    }
    return temp;
}

// prints out http error response codes
void error_print(int err, int socket){
	if(err == 400){
		string content = "Content-Length: " + to_string(0) + "\r\n\r\n";
		string error = "HTTP/1.1 400 Bad Request\r\n" + content;
    	sending_packet(socket, error);
	}
	if(err == 403){
		string content = "Content-Length: " + to_string(0) + "\r\n\r\n";
    	string error = "HTTP/1.1 403 Forbidden\r\n" + content;
    	sending_packet(socket, error);
	}
	if(err == 404){
		string content = "Content-Length: " + to_string(0) + "\r\n\r\n";
    	string error = "HTTP/1.1 404 Not Found\r\n" + content;
    	sending_packet(socket, error);
	}
}

int error_catch(string head){
    int temp;
    if((temp = head.find("404")) >= 0){
        return 1;
    }
    return 0;
}

void catch_range(string line, int *start, int *end){
	int temp;
	string temp_string;
	if((temp = line.find("Content-Range")) >= 0){
		temp_string = line.substr(temp);
		int first = (temp_string.find("Content-Range:") + 15);// Used to get filesize
		string temp2 = temp_string.substr(first);
		int middle = (temp2.find("-") + first);
		int last = ((temp_string.find("/")) - middle) -1 ;
		string ftemp = temp_string.substr(first,last);
		// printf("%d, %d\n", middle - first, last);
		*start = stoi(temp_string.substr(first, middle - first));
		*end = stoi(temp_string.substr(middle + 1, last)); 
	}else{
		*start = -1;
		*end = -1;
	}
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
// Creates appropriate GET and HEAD HTTP Request

void writing(string temp, int begin, int* local_written){
    if(written == 0){
        remove(filename.c_str());
    }
    int file_num = open(filename.c_str(), O_WRONLY | O_CREAT, 0777);
    *local_written += pwrite(file_num, temp.c_str(), temp.length(), begin);
    close(file_num);

}

void http_requests(int sock, int type, string file, string hostname){
    string temp = "";
    if(type == 0){
        temp += "HEAD " + file + " HTTP/1.1\r\nHost: " + hostname + "\r\n\r\n" ;
    }
    if(type == 1){
        temp += "GET " + file + " HTTP/1.1\r\nHost: " + hostname + "\r\n\r\n";
    }
    // printf("%s\n", temp.c_str());
    sending_packet(sock, temp);

}

string get_head(string full_message, int * beginning, int* end){
    string temp;
    int first = full_message.find("\r\n\r\n") + 4;
    int local_length = catch_length(full_message.substr(0, first));
    catch_range(full_message.substr(0, first), beginning, end);
    // printf("%d\n", local_length);
    temp = full_message.substr(first, local_length);
    return temp;
}

void *establish_connection(void *){
    int socket;
    // int numbytes;
    // char c;
    string hostname;
    string temp = "";
    string request = "";
    sockaddr server;


    while(1){
		// Consumer code
		sem_wait(&full);
		pthread_mutex_lock(&mutex1);
        socket = buff[out];
        server = addr_buff[out];
        // hostname = host_buff[out];
        out = (out + 1) % num_args;
        pthread_mutex_unlock(&mutex1);
        sem_post(&empty);

        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        int beginning, end;
        try{
            while(written < length && setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) >= 0){
                int start = 0;
                temp = "";
                request = "";
                // bool done = false;
                
                pthread_mutex_lock(&mutex_offest);
                start = size_of_chunks*offset;
                offset = (offset + 1) % num_args;
                pthread_mutex_unlock(&mutex_offest);
                // printf("%d", offset);

                if((length - written) < size_of_chunks){
                    size_of_chunks = length - written;
                    request += "GET " + filename + " HTTP/1.1\r\nHost: " + "127.0.0.1" + "\r\n" + "Content-Range: " + to_string(written) + "-" + to_string(length) + "/" + to_string(length) + "\r\n\r\n";
                }else{
                    request += "GET " + filename + " HTTP/1.1\r\nHost: " + "127.0.0.1" + "\r\n" + "Content-Range: " + to_string(start) + "-" + to_string(start + size_of_chunks) + "/" + to_string(length) + "\r\n\r\n";
                }

                sending_packet(socket, request);
                temp = recieve_packets(socket);
                if(temp == ""){
                    break;
                }
                temp = get_head(temp, &beginning, &end);
                // printf("%s", temp.c_str());

                if(beginning <= written){
                    pthread_mutex_lock(&mutex_write);
                        writing(temp, start, &written);
                    pthread_mutex_unlock(&mutex_write);
                    temp == "";
                }
            }
            if(written != length){
                warn("Connection to one of the servers has been lost"); 
            }
            if(written == length){
                exit(1);
            }
        }catch(...){
            string content = "Content-Length: " + to_string(0) + "\r\n\r\n";
		    string header = "HTTP/1.1 500 Created\r\n" + content;
			sending_packet(socket, header);
        }
    }
}

int main(int argc, char * argv[]){
    //Checks for appropriate number of args
    if(argc < 4){
        warn("Insufficient number of arguements givin");
        exit(0);
    }
    

    char * ip_file = argv[1];
    num_args = atoi(argv[2]);
    filename = argv[3];
    string hostname;
    string port;
    string temp = "";
    int new_fd;


    
    try{

        struct addrinfo hints, *addrs;
        memset(&hints, 0,sizeof hints);
        hints.ai_family=AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;

        
        buff = (int*) malloc(sizeof(int)*(num_args*800));
        // host_buff = (string*) malloc(sizeof(string)*(num_args*800));

        //cout << 2 << "\n";


        ifstream ips(ip_file);
        string line;

        if(!ips.is_open()){
            warn("Ip file does not exist");
            exit(0);
        }

        sem_init(&empty, 0, num_args);
	    sem_init(&full, 0, 0);

        addr_buff = (struct sockaddr *) malloc(sizeof(struct sockaddr *)*(num_args*800));

        while(written != length){
            while(getline(ips, line)){
                // cout << line;
                hostname = line.substr(0, line.find(" "));
                port = line.substr((line.find(" ") + 1));
                
                // cout << port;
                // printf("%s", hostname.c_str());

                getaddrinfo(hostname.c_str(), port.c_str(), &hints, &addrs);
                new_fd = socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol);

                int does_it_work;
                
                if((does_it_work = connect(new_fd,addrs->ai_addr,addrs->ai_addrlen)) == -1){
                    new_fd = 0;
                }

                // char c = 'a';
                // string temp = "";
                // while(1){
                //     temp += c;
                //     read(0, &c, 1);
                //     printf("%c", c);
                //     if(c == '9'){
                //         break;
                //     }
                // }
                // sendto(new_fd, temp.c_str(), temp.length(), 0, (struct sockaddr *)NULL, 0);

                if(!first_connect && new_fd > 0){ 
                    struct timeval tv;
                    tv.tv_sec = 1;
                    tv.tv_usec = 0;
                    setsockopt(new_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
                    http_requests(new_fd, 0, filename, hostname);
                    
                    // char buffin[1024];
                    // recvfrom(new_fd, buffin, 1024, 0, (struct sockaddr *)NULL, NULL);
                    temp = recieve_packets(new_fd);
                    length = catch_length(temp);
                    if(length == -1){
                        new_fd = 0;
                    }
                    size_of_chunks = (length / num_args);
                    // cout << length << "\n";
                    first_connect = true;
                }

                if(new_fd > 0){

                    // printf("%d\n", new_fd);
                    // printf("1");

                    pthread_t tidsi;
			        pthread_create(&tidsi, NULL, establish_connection, NULL);

                    sem_wait(&empty);
                    pthread_mutex_lock(&mutex1);
                    buff[in] = new_fd;
                    addr_buff[in] = *(addrs->ai_addr);
                    // host_buff[in] = hostname;
                    in = (in + 1) % num_args;

                    pthread_mutex_unlock(&mutex1);
                    sem_post(&full);
                }
            }

        }

        

        
        // exit(1);
    }catch(...){
        warn("Warning internal server error closing connections");
    }
}
