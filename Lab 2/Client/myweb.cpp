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
using namespace std;

int num_args = 4;
int out = 0;
int in = 0;
int offset = 0;
int length = -1;
sem_t empty, full;
int * buff;
string * host_buff;
int written = 0;
int size_of_chunks;
bool first_connect = false;
string filename;
//int patch_offset= 0;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

// prints out http error response codes
void error_print(int err, int socket){
	if(err == 400){
		string content = "Content-Length: " + to_string(0) + "\r\n\r\n";
		string error = "HTTP/1.1 400 Bad Request\r\n" + content;
    	send(socket, error.c_str(), error.length(), 0);
	}
	if(err == 403){
		string content = "Content-Length: " + to_string(0) + "\r\n\r\n";
    	string error = "HTTP/1.1 403 Forbidden\r\n" + content;
    	send(socket, error.c_str(), error.length(), 0);
	}
	if(err == 404){
		string content = "Content-Length: " + to_string(0) + "\r\n\r\n";
    	string error = "HTTP/1.1 404 Not Found\r\n" + content;
    	send(socket, error.c_str(), error.length(), 0);
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

void writing(string c, int begin, int* local_written){
    if(written == begin){
        int file_num = open(filename.c_str(), O_WRONLY | O_CREAT, 0777);
        *local_written += pwrite(file_num, c.c_str(), c.length(), begin);
        close(file_num);
    }

}

void http_requests(int sock, int type, string file, string hostname){
    string temp = "";
    if(type == 0){
        temp += "HEAD " + file + " HTTP/1.1\r\nHost: " + hostname + "\r\n\r\n" ;
    }
    if(type == 1){
        temp += "GET " + file + " HTTP/1.1\r\nHost: " + hostname + "\r\n\r\n";
    }
    send(sock, temp.c_str(), temp.length(), 0);

}

int head_parse(int sock){
    int numbytes= 0;
    string temp = "";
    char c;
    int end_header = 0;
    while((numbytes = recv(sock, &c, 1, 0)) != 0){
        // printf("%c", c);
        if(end_header != 1){
            temp += c;
        }
        // Checks for end of header
        if(end_header == 0 && temp.length() > 3 && temp.substr(temp.length() - 4) == "\r\n\r\n"){ //Checks for end of header
            end_header = 1;
        }
        if(end_header == 1){
            return catch_length(temp);
        }
    }
    return -1;
}

void *establish_connection(void *){
    int socket;
    int end_header = 0;
    int numbytes;
    char c;
    string hostname;
    string temp = "";
    string request = "";


    while(1){
		// Consumer code
		sem_wait(&full);
		pthread_mutex_lock(&mutex1);
		socket = buff[out];
        hostname = host_buff[out];
		out = (out + 1) % num_args;
		pthread_mutex_unlock(&mutex1);
		sem_post(&empty);

        int local_length = -1;
        int local_written = 0;
        bool written_file = false;

        int chunk = size_of_chunks*offset;
        offset = (offset + 1) % num_args;

        if((size_of_chunks + chunk) <= length){
            request += "GET " + filename + " HTTP/1.1\r\nHost: " + hostname + "\r\n" + "Content-Range: " + to_string(chunk) + "-" + to_string(chunk + size_of_chunks) + "/" + to_string(length) + "\r\n\r\n";
        }else{
            int right_size = (size_of_chunks + chunk) - length;
            request += "GET " + filename + " HTTP/1.1\r\nHost: " + hostname + "\r\n" + "Content-Range: " + to_string(chunk) + "-" + to_string(chunk + right_size) + "/" + to_string(length) + "\r\n\r\n";
        }

        send(socket, request.c_str(), request.length(), 0);

        // Starts recieving response from server
        while((numbytes = recv(socket, &c, 1, 0)) != 0){
            temp += c;
            printf("%c", c);
            // Checks for end of header
            if(end_header == 0 && temp.length() > 3 && temp.substr(temp.length() - 4) == "\r\n\r\n"){ //Checks for end of header
                end_header = 1;
                local_length = catch_length(temp);
                temp = "";
            }
            if(end_header == 1 && temp.length() > 3 && temp.substr(temp.length() - 4) == "\r\n"){
                break;
            }
        }
        while(!written_file){
            if(chunk < written){
                break;
            }
            writing(temp, chunk, &local_written);
            written += local_written;
            if(local_written == local_length){
                written_file = true;
            }
            if(written == length){
                exit(1);
            }
        }
    }
}

int main(int argc, char * argv[]){
    //Checks for appropriate number of args
    if(argc < 2){
        warn("Insufficient number of arguements givin");
    }
    
    //cout << "here\n";
    

    char * ip_file = argv[1];
    num_args = atoi(argv[2]);
    filename = argv[3];
    //printf("%s, %d, %s\n", ip_file, num_args, filename.c_str());
    // int f = open(ip_file, O_RDONLY);
    // char c;
    string hostname;
    string port;
    string temp = "";
    int new_fd;
    // int size_of_chunks = 4;
    // int length= -1;
    // int chunk;
    //cout << 1 << "\n";


    
    try{

        // struct addrinfo hints, *addrs;
	    // struct sockaddr_storage their_addr;
	    // socklen_t addr_size;
        // memset(&hints, 0,sizeof hints);
        // hints.ai_family=AF_UNSPEC;
        // hints.ai_socktype = SOCK_STREAM;
        // int count = 0;
        
        
        buff = (int*) malloc(sizeof(int)*(num_args*800));
        host_buff = (string*) malloc(sizeof(string)*(num_args*800));

        //cout << 2 << "\n";

        struct sockaddr_in servaddr;

        ifstream ips(ip_file);
        string line;
        int first, last;
        

        while(getline(cin, line)){
            cout << line;
            // first = line.find(" ");
            // // last = line.find("")
            // hostname = line.substr(0, first);
            // port = line.substr(first + 1);

            // cout << hostname << " " << port << "\n";

            new_fd=socket(AF_INET,SOCK_STREAM,0);
            bzero(&servaddr,sizeof servaddr);
        
            servaddr.sin_family=AF_INET;
            servaddr.sin_port=htons(12345);
        
            inet_pton(AF_INET,"127.0.0.1",&(servaddr.sin_addr));
            
            

            // cout << 3 << "\n";
            if(!first_connect && new_fd > 0){
                connect(new_fd,(struct sockaddr *)&servaddr,sizeof(servaddr));
                
                http_requests(new_fd, 0, filename, "127.0.0.1");
                length = head_parse(new_fd);
                cout << length << "\n";
                if(length == -1){
                    new_fd = 0;
                }
                // size_of_chunks = (length / num_args);
                // first_connect = true;
            }
            // if(new_fd > 0){

			// 	//printf("%d\n", new_fd);

            //     pthread_t tidsi;
            //     pthread_create(&tidsi, NULL, establish_connection, NULL);

			// 	sem_wait(&empty);
			// 	pthread_mutex_lock(&mutex1);

            //     connect(new_fd,addrs->ai_addr,addrs->ai_addrlen);
			// 	buff[in] = new_fd;
            //     host_buff[in] = hostname;
			// 	in = (in + 1) % num_args;

			// 	pthread_mutex_unlock(&mutex1);
			// 	sem_post(&full);
			// }
        }
        // exit(1);
    }catch(...){
        warn("Warning internal server error closing connections");
    }
}
