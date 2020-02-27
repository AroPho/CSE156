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
// #include <chrono>
// #include <ctime>
// #include <limits.h>
// #include <linux/unistd.h>       /* for _syscallX macros/related stuff */
// #include <linux/kernel.h>       /* for struct sysinfo */
using namespace std;

int guard(int n, char * err) { if (n == -1) { perror(err); exit(1); } return n; }

// void printing(int f, int socket ){
//     string temp;
//     int size;
//     char c;
//     while(read(f, &c, 1) != 0){
//     	temp += c;
//     }
// }

// void error_print(int error, int scoket){

// }

// void uptime_command(int socket){
// 	struct sysinfo s_info;

// 	const time_t now = time(nullptr) ;
// 	tm calendar_time = *localtime( addressof(now) ) ;

// 	string hour = to_string(calendar_time.tm_hour);
// 	string minutes = to_string(calendar_time.tm_min);
// 	int uptime_min = s_info.uptime / 60;
// 	int uptime_hour = uptime_min / 60;
// 	int uptime_day = uptime_hour / 24;

// 	if(uptime_hour == 0 && uptime_day == 0){
// 		string out = hour + ":" + minutes + "  up " + uptime_min + " mins, load averages: " + s_info.load[1] + " " + s_info.load[2], " " + s_info.load[3];
// 	}
// 	if(uptime_hour != 0 && uptime_day == 0){
// 		string out = hour + ":" + minutes + "  up " + uptime_hour + " hours, load averages: " + s_info.load[1] + " " + s_info.load[2], " " + s_info.load[3];
// 	}
// 	if(uptime_day != 0){
// 		string out = hour + ":" + minutes + "  up " + uptime_day + " days, load averages: " + s_info.load[1] + " " + s_info.load[2], " " + s_info.load[3];
// 	}
// 	send(socket, out.c_str(), out.length(), 0);
// }

// void date_command(int socket){

// 	auto end = chrono::system_clock::now();
// 	time_t end_time = chrono::system_clock::to_time_t(end);
// 	string current_time = ctime(&end_time);
// 	send(socket, current_time.c_str(), current_time.length(), 0);
	
// }

// void pwd_command(int socket){
// 	char cwd[PATH_MAX];
// 	if (getcwd(cwd, sizeof(cwd)) != NULL) {
//        send(socket, cwd, sizeof(cwd), 0);
//    }
// }

// void who_command


// void cat_command(string temp, int socket){
// 	int first = temp.find(" ");
// 	int last = temp.find("\0") - first - 1;
// 	// if(temp.find("/") == 0){
// 	// 	temp = temp.substr(1);
// 	// }
// 	//printf("%s\n", temp.c_str());	
// 	int f = open(temp.c_str(), O_RDONLY);
// 	if(errno == 13){ // Insufficient Permissions
//     	error_print(403, socket);
//     }
// 	if(f == -1){
// 		error_print(404, socket);
//    	}else{
// 		printing(f, socket); // Calls file to start sending client data
// 	}
// }


// void determine_command(string command, int socket){
// 	string temp = command.substr(0, 8);
// 	if(temp.find("cat") != -1){
// 		cat_command(command, socket);
// 	}if(temp.find("date") != -1){
// 		date_command(command, socket);
// 	}if(temp.find("df -h") != -1){
// 		return 3;
// 	}if(temp.find("du") != -1){
// 		return 4;
// 	}if(temp.find("echo") != -1){
// 		return 5;
// 	}if(temp.find("free") != -1){
// 		return 6;
// 	}if(temp.find("last") != -1){
// 		return 7;
// 	}if(temp.find("ps aux") != -1){
// 		return 8;
// 	}if(temp.find("pwd") != -1){
// 		pwd_command(command, socket);
// 	}if(temp.find("set") != -1){
// 		return 10;
// 	}if(temp.find("tty") != -1){
// 		return 11;
// 	}if(temp.find("uname -a") != -1){
// 		return 12;
// 	}if(temp.find("uptime") != -1){
// 		uptime_command(command, socket);
// 	}if(temp.find("who") != -1){
// 		return 14;
// 	}if(temp.find("exit") != -1){
// 		return 15;
// 	}
// 	return -1;
// }

void establish_connnection(int sock){
	int numbytes;
	char c;
	string temp = "";
	char buffer[128];
	string result = "";
	FILE* pipe;
	string error_command = "Command not Found\r\n";
	int result_int;
	try{
	while((numbytes = recv(sock, &c, 1, 0)) != 0){
		temp += c;
		if(temp.length() > (long) 1 && temp.substr(temp.length() - 2) == "\r\n"){
			// determine_command(temp, sock);
			printf("%s\n", temp.c_str());
			// Open pipe to file
			pipe = popen(temp.substr(0, temp.length() - 2).c_str(), "r");
			if (!pipe) {
				string pipe_error = ""
				// send(sock, error_command.c_str(), error_command.length(), 0);
				warn("fuck");
			}
			// read till end of process:
			while (!feof(pipe)) {
				printf("1");
				// use buffer to read and add to result
				if (fgets(buffer, 128, pipe) != NULL){
					result += buffer;
				}
			}
			// while (fgets(buffer, 128, pipe) != NULL) {
			// 	std::cout << "Reading..." << std::endl;
			// 	result += buffer;
			// }
			// printf("%s\n", result.c_str());
			// if((result_int = result.find("command not found")) <  0){
			// 	send(sock, error_command.c_str(), error_command.length(), 1);
			// }
			temp = "";
			// if((result_int = result.find("command not found")) >=  0){
			printf("here");
			if(result != ""){
				result += "\r\n";
				send(sock, result.c_str(), result.length(), 0);
			}else{
				send(sock, error_command.c_str(), error_command.length(), 0);
			}
			// }

			printf("okay");
			result = "";
			pclose(pipe);
		}

	}
	}catch(...){
		warn("help");
	}
	exit(0);
}

int main(int argc, char * argv[]) {
//   int listen_fd = guard(socket(PF_INET, SOCK_STREAM, 0), "Could not create TCP socket");
//   printf("Created new socket %d\n", listen_fd);
//   guard(listen(listen_fd, 100), "Could not listen on TCP socket");
//   struct sockaddr_in listen_addr;
//   socklen_t addr_len = sizeof(listen_addr);
//   guard(getsockname(listen_fd, (struct sockaddr *) &listen_addr, &addr_len), "Could not get socket name");
//   printf("Listening for connections on port %d\n", ntohs(listen_addr.sin_port));

	if(argc != 2){
		warn("Incorrect number of args");
		exit(0);
	}

	struct sockaddr_in servaddr;
	struct sockaddr_storage their_addr;
	socklen_t addr_size;
 
    int main_socket = socket(AF_INET, SOCK_STREAM, 0);

	char* port = argv[1];

    // struct timeval tv;
    // tv.tv_sec = 1;
    // tv.tv_usec = 0;
    // setsockopt(main_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
	
	//Create Listen Socket
    bzero( &servaddr, sizeof(servaddr));
 
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(atoi(port));
 
    bind(main_socket, (struct sockaddr *) &servaddr, sizeof(servaddr));
	listen (main_socket, 16);
	int new_fd;
	// char c;
	string fork_error = "Could not fork";
	string recv_error = "Could not recv on TCP connection";
	while(main_socket > 0){
		new_fd = accept(main_socket, (struct sockaddr *)&their_addr, &addr_size);
		//parse_recv(new_fd);
		if(new_fd > 0){
			printf("Got new connection %d\n", new_fd);
			if (guard(fork(), (char *) fork_error.c_str()) == 0) {
				// while(numbytes != 0){
				// 	recv(new_fd, &c, 1, 0);
				// 	printf("%c", c);
				// }
				establish_connnection(new_fd);
				// Child takes over connection; close it in parent
				close(new_fd);
			// read(0, &c, 1024);
			// send(new_fd, &c, 1, 0);	
			}
		}
	}
	// return 0;
}
