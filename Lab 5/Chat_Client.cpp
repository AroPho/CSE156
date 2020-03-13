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
#include <fstream>
using namespace std;

bool connection_bool = false;
int connection_socket;
string client_name;
bool quit = false;

int guard(int n, char * err) { if (n == -1) { perror(err); exit(1); } return n; }

void my_handler(int s){
    quit = true;
}

void *p2p_send(void *){
    // struct sockaddr_in cliaddr;
    // socklen_t addr_size;

    // struct sigaction sigIntHandler;

    // sigIntHandler.sa_handler = my_handler;
    // sigemptyset(&sigIntHandler.sa_mask);
    // sigIntHandler.sa_flags = 0;

    // sigaction(SIGINT, &sigIntHandler, NULL);

    int sock = connection_socket;
    string name = client_name;
    //char input[1024];
    // int n;
    string input;
    string sending = "";
    // bzero(input, 1024);
    while(connection_bool && !quit){
        cout << name << "> ";
        sending = name + ": ";
        getline(cin, input);
        if(input == "/quit"){
            connection_bool = false;
            quit = true;
        }
        input += "\r\n";
        sending += input;
        send(sock, input.c_str(), input.length(), 0);
    }
    return NULL;

}

void p2p_connect_connect(string command){
    struct sockaddr_in servaddr;
    string line = command.substr(4);
    string name = client_name;

    string hostname = line.substr(0, line.find(" "));
    string port = line.substr((line.find(" ") + 1));
    // printf("%s %s\n", hostname.c_str(), port.c_str());

    bzero(&servaddr, sizeof(servaddr)); 
    servaddr.sin_addr.s_addr = inet_addr(hostname.c_str()); 
    servaddr.sin_port = htons(stoi(port)); 
    servaddr.sin_family = AF_INET; 
    
    // create datagram socket 
    int new_fd = socket(AF_INET, SOCK_STREAM, 0);

    int does_it_work;
    
    if((does_it_work = connect(new_fd,(struct sockaddr *)&servaddr, sizeof(servaddr))) == -1){
        // printf
        new_fd = 0;
    }
    // printf("%d\n", new_fd);
    if(new_fd != 0){
        int n;
        // string ping = "ping\r\n\r\n";
        // send(new_fd, ping.c_str(), ping.length(), 0);
        // string fork_error = "Could not fork";

        connection_bool = true;
        connection_socket = new_fd;

        pthread_t tidsb;
		pthread_create(&tidsb, NULL, p2p_send, NULL);

        char c;
        string temp = "";
        printf("client_connect\n");
        while((n = recv(new_fd, &c, 1, MSG_DONTWAIT)) != 0 && connection_bool){
            if(n == 1){
                temp += c;
            }
            if(quit == true || temp == "/quit\r\n"){
                // printf("2\n");
                string quitting = "/quit\r\n";
                send(new_fd, quitting.c_str(), quitting.length(), 0);
                break; 
            }
            if(temp.length() > 2 && temp.substr(temp.length() -2) == "\r\n"){
                // printf("\n%s\n", temp.substr(0, temp.length() - 2).c_str());
                cout << "\n" << temp.substr(0, temp.length() - 2).c_str() << "\n";
                cout << name << "> ";
                temp = "";
            }
            
        }
        connection_bool = false;
        // establish_connnection(new_fd);
    }
}

void p2p_wait_connect(int sock){
    int numbytes;
    // printf("ayo");
    char c;
    string temp;
    string name = client_name;
    // n = recvfrom(sock, &input, 1024, 0, (struct sockaddr *)&cliaddr, &addr_size);
    // printf("%s\n", input);

    pthread_t tidsb;
	pthread_create(&tidsb, NULL, p2p_send, NULL);

    while((numbytes = recv(sock, &c, 1, MSG_DONTWAIT)) != 0 && connection_bool){
        if(numbytes == 1){
            temp += c;
        }
        // printf("%c", c);
        if(quit == true || temp == "/quit\r\n"){
            // printf("2\n");
            
            string quitting = "/quit\r\n";
            send(sock, quitting.c_str(), quitting.length(), 0);
            break; 
        }
        if(temp.length() > 2 && temp.substr(temp.length() - 2) == "\r\n"){
            // printf("\n%s\n", temp.substr(0, temp.length() - 2).c_str());
            cout << "\n" << temp.substr(0, temp.length() - 2).c_str() << "\n";
            cout << name << "> ";
            temp = "";
        }
    }
    // printf("Shuting Down Connection to Client");
    connection_bool = false;
}

void wait_recieve(int sock){
    // char c;
    // int numbytes;
    string temp = "";
    // printf("here\n");

    struct sockaddr_in servaddr;
    // struct sockaddr_storage their_addr;

    string no_flag = "could not get flags on TCP listening socket";
    string blocking = "could not set TCP listening socket to be non-blocking";
    char * char_block = (char*) blocking.c_str();

    int main_socket = socket(AF_INET, SOCK_STREAM, 0);
    int flags = guard(fcntl(main_socket, F_GETFL), (char*) no_flag.c_str());
    guard(fcntl(main_socket, F_SETFL, flags | O_NONBLOCK), char_block);
    int new_fd;

    //Create Listen Socket
    bzero( &servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(0);

    bind(main_socket, (struct sockaddr *) &servaddr, sizeof(servaddr));
    listen (main_socket, 16);

    socklen_t len = sizeof(servaddr);
    struct sockaddr_storage their_addr;

    getsockname(main_socket, (struct sockaddr *) &servaddr, &len);
    string port = "Port: " + to_string(ntohs(servaddr.sin_port)) + "\r\n\r\n";
    send(sock, port.c_str(), port.length(), 0);



    // printf("here\n");

    while(main_socket > 0){
        new_fd = accept(main_socket, (struct sockaddr *)&their_addr, &len);
        // printf("%d\n", new_fd);
        if(new_fd > 0){
            connection_socket = new_fd;
            connection_bool = true;
            // printf("%d\n", new_fd);
            p2p_wait_connect(new_fd);
            break;
        }
        if(quit == true){
            // printf("2\n");
            string quitting = "/quit\r\n\r\n";
            send(sock, quitting.c_str(), quitting.length(), 0);
            break;
        }
    }
    printf("Shuting Down Connection to Client\n");
    string quitting = "/quit\r\n\r\n";
    send(sock, quitting.c_str(), quitting.length(), 0);
    close(main_socket);

}

bool inputAvailable() {
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    return (FD_ISSET(0, &fds));
}

void *wait(void *){
    string input;
    string name = client_name;

    // struct sigaction sigIntHandler;

    // sigIntHandler.sa_handler = my_handler;
    // sigemptyset(&sigIntHandler.sa_mask);
    // sigIntHandler.sa_flags = 0;

    // sigaction(SIGINT, &sigIntHandler, NULL);

    // int c;
    // printf("%s> ", name.c_str());
    while(1){
        //printf("%lu", input.length());
        if(inputAvailable()){
            getline(cin, input);
            //printf("%s> ", name.c_str());
        }
        
        // printf("here\n");
        if(input == "/quit"){
            // printf("1\n");
            quit = true;
            return NULL;
        }
        if(input != "/quit" && input.length() != 0) {
            printf("Command %s not recognized\n", input.c_str()); 
            cout << name;
            input = "";
        }
        if(connection_bool == true){
            return NULL;
        }
    }
    
}


// Recieves data from Server
void recieving(int socket){
    int numbytes;
    char c;
    string temp = "";
    
    while((numbytes = recv(socket, &c, 1, 0)) != 0){
        temp += c;
        if(temp.length() > 3 && temp.substr(temp.length() - 4) == "\r\n\r\n"){ //Checks for end of header
            break;
        }
    }
    if(numbytes == 0){
        printf("Connection to server has been closed\n");
        exit(0);
    }

    if(temp.substr(0, 7) == "Error: "){
        printf("User not found");
    }

    // Info -> Chat
    if(temp.substr(0, 4) == "Ip: "){
        //printf("%s", temp.substr(0, temp.length() - 4).c_str());
        p2p_connect_connect(temp.substr(0, temp.length() - 4));
        quit = false;
        string quitting = "/quit\r\n";
        send(socket, quitting.c_str(), quitting.length(), 0);

    }

    //Info -> Wait
    if(temp == "wait\r\n\r\n"){
        pthread_t tidsc;
		pthread_create(&tidsc, NULL, wait, NULL);
        // printf("%s> ", client_name.c_str());
        // printf("here");
        wait_recieve(socket);
        quit = false;
    }

    // Info -> Info
    if(temp != "wait\r\n\r\n" && temp.substr(0, 4) != "Ip: "){
        printf("%s\n", temp.substr(0,temp.length() - 4).c_str());
    }
    
}

void first_contact(int sock){
    string temp = client_name + "\r\n\r\n";
    send(sock, temp.c_str(), temp.length(), 0);
    int numbytes;
    char c;
    temp = "";
    while((numbytes = recv(sock,&c, 1, 0) != 0)){
        temp += c;
        if(temp.length() >= 4 && temp.substr(temp.length() - 4) == "\r\n\r\n"){
            // printf("%s\n", temp.c_str());
            if(temp == "TAKEN\r\n\r\n"){
                printf("Client ID is already taken");
                exit(0);
            }
            break;
        }
    }
}

int main(int argc, char * argv[]){
    //Checks for appropriate number of args

        if(argc != 4){
            warn("Incorrect number of arguements");
            exit(0);
        }

        struct addrinfo hints, *addrs;
        memset(&hints, 0,sizeof hints);
        hints.ai_family=AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        string hostname = argv[1];
        string port = argv[2];
        client_name = argv[3];
        // printf("%s %s\n", hostname.c_str(), port.c_str());
        
        // cout << port;
        // printf("%s", hostname.c_str());
        int new_fd;
        getaddrinfo(hostname.c_str(), port.c_str(), &hints, &addrs);
        new_fd = socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol);
        int confirm = connect(new_fd, addrs->ai_addr,addrs->ai_addrlen);
        if(confirm == -1){
            warn("Unable to connect to server. Please try again or try different IP");
            exit(0);
        }
        
        first_contact(new_fd);
        

    try{
        // char c;
        string input;
        while(1){
            // Gets input from stdin
            printf("%s> ", client_name.c_str());
            getline(cin, input);
            
            if(input == "/quit"){
                exit(0);
            }

            // Prevents terminal texts from being used
            input += "\r\n\r\n";
            send(new_fd, input.c_str(), input.length(), 0);
            recieving(new_fd);       
        }

    }catch(...){
        warn("Warning internal server error closing connections");
        string content = "Content-Length: " + to_string(0) + "\r\n\r\n";
        string header = "HTTP/1.1 500 Created\r\n" + content;
        send(new_fd, header.c_str(), header.length(), 0);
    }
}