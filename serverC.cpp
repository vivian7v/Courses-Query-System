#include <iostream>
#include <map>
#include <fstream>
#include <cstdlib>
#include <iterator>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <thread>
#include <pthread.h>
using namespace std;

#define BUFF_MAXSIZE 1024
#define serverM_UDP_PortNumber 24770
#define serverC_UDP_PortNumber 21770
#define IP_Address "127.0.0.1"
char IP[] = IP_Address;

//Global Variables
string file_str = "";
std::map<string, string> users_map;
string file_path = "./cred.txt";
int socketfd;
struct sockaddr_in server_addr;
struct sockaddr_in client_addr;
string udp_recv_message = "";
string username = "";
string password = "";

//function declearance
void readTxt(string file_path);
void split(string file_strs);
void upd_recv(int portNumber);
void upd_respond(int socketfd);
void udp_server(int server_portNumber, int client_portNumber);
void udp_single_send(char message[], const struct sockaddr_in *addr);


int main(){

	//Inform that the serverC is Running
    cout<<"The ServerC is up and running using UDP on port "<<serverC_UDP_PortNumber<<"."<<endl;
	
	thread thread_udp(udp_server, serverC_UDP_PortNumber, serverM_UDP_PortNumber);
	thread_udp.detach();

	//read cred.txt
	readTxt(file_path);
	cout<<endl;
	std::map<string, string>::iterator it;


	//deal with authentication
	while(1){	
		if(udp_recv_message != ""){
			for(int i = 0; i < udp_recv_message.size(); i++){
				if(udp_recv_message[i] == ','){
					username = udp_recv_message.substr(0, i);
					password = udp_recv_message.substr(i + 1, udp_recv_message.size() - 1);
				}
			}
			
			cout<<"The ServerC received an authentication request from the Main Server."<<endl;

			if(users_map.count(username) == 0){
				char username_fail_message[] = "Username Does Not Exist";
				udp_single_send(username_fail_message, &client_addr);
				udp_recv_message.clear();
			}
			else if(users_map[username] != password){
				char password_fail_message[] = "Password Does Not Match";
				udp_single_send(password_fail_message, &client_addr);
				udp_recv_message.clear();
			}
			else{
				char correct_message[] = "ID Pass";
				udp_single_send(correct_message, &client_addr);
				udp_recv_message.clear();
			}

			cout<<"The ServerC finished sending the response to the Main Server."<<endl;
		}
	}

	return 0;	
}

//code cited
void readTxt(string file_path){
    ifstream infile;
    infile.open(file_path.data());

    if(!infile.is_open()){
        cout<<"Can not open this file"<<endl;
    }

	//cout<<"Read Text Successfully!"<<endl;

    string s;

    while(getline(infile, s)){
		split(s);
        //cout<<s<<endl;
    }

    infile.close();
}

void split(string file_strs){
	string str1 = "";
	string str2 = "";
	for(int i = 0; i < file_strs.size(); i++){
		if(file_strs[i] == ','){
			str1 = file_strs.substr(0, i);
			str2 = file_strs.substr(i + 1, file_strs.size() - i - 2);
			break;
		}
	}
	users_map[str1] = str2;
};

void udp_single_send(char message[], const struct sockaddr_in *addr){
	sendto(socketfd, message, strlen(message), 0, (struct sockaddr*)addr, sizeof(struct sockaddr_in));
}

void upd_respond(int socketfd){
    struct sockaddr_in addr;
    unsigned int addrlen, n;
    char message[BUFF_MAXSIZE];

    while(1){
        bzero(message, sizeof(message));
        addrlen = sizeof(struct sockaddr);
        n = recvfrom(socketfd, message, BUFF_MAXSIZE, 0, (struct sockaddr*)&addr, &addrlen);

        message[n] = 0;
		udp_recv_message = message;
    }
}

void upd_recv(int portNumber){
    int socketfd;
    struct sockaddr_in server_addr;

    socketfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(socketfd == -1){
        cout<<"Socket Error"<<endl;
        exit(1);
    }

    bzero(&server_addr, sizeof(sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(portNumber);

    if(bind(socketfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in)) < 0){
        cout<<"Bind Error"<<endl;
        exit(1);
    }

    upd_respond(socketfd);
    close(socketfd);
}

void udp_server(int server_portNumber, int client_portNumber){
	socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(socketfd == -1){
		cout<<"Socket Error"<<endl;
		exit(1);
	}

	bzero(&server_addr, sizeof(sockaddr_in));
	bzero(&client_addr, sizeof(sockaddr_in));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(server_portNumber);

	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(client_portNumber);


    if(bind(socketfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in)) < 0){
        cout<<"Bind Error"<<endl;
        exit(1);
    }

	if(inet_aton(IP, &client_addr.sin_addr) == -1){
		cout<<"Ip Error"<<endl;
		exit(1);
	}

	upd_respond(socketfd);

	close(socketfd);
}
