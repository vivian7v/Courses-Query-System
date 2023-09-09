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
#include <vector>
#include <pthread.h>
#include <thread>
using namespace std;

//Global Variables
#define BUFF_MAXSIZE 1024
#define serverM_UDP_PortNumber 24770
#define serverEE_PortNumber 22770
#define IP_Address "127.0.0.1"
char IP[] = IP_Address;
#define Credit 0
#define Professor 1
#define Days 2
#define CourseName 3
string file_path = "./ee.txt";
std::map<string, vector<string>> courses_map;
int socketfd;
struct sockaddr_in server_addr;
struct sockaddr_in client_addr;
string udp_recv_message = "";
string course_name = "";
string course_category = "";

//function declearance
void readTxt(string file_path);
void split(string file_strs);
void upd_recv(int portNumber);
void upd_respond(int socketfd);
void udp_server(int server_portNumber, int client_portNumber);
void udp_single_send(char message[], const struct sockaddr_in *addr);
int category_selection(string course_category);

int main(){
    cout<<"The ServerEE is up and running using UDP on port "<<serverEE_PortNumber<<"."<<endl;
    thread thread_udp(udp_server, serverEE_PortNumber, serverM_UDP_PortNumber);
    thread_udp.detach();
    
    readTxt(file_path);

	while(1){	
		if(udp_recv_message != ""){

			for(int i = 0; i < udp_recv_message.size(); i++){
				if(udp_recv_message[i] == ','){
					course_name = udp_recv_message.substr(0, i);
					course_category = udp_recv_message.substr(i + 1);
				}
			}

            
            cout<<"The ServerEE receiveed a request from the Main Server about the " + course_category + " of " + course_name + "."<<endl;

			if(courses_map.count(course_name) == 0){
                string temp = "Didn't find the course: " + course_name;
                cout<<temp<<"."<<endl;
				char courser_name_fail_message[1024];
                temp = temp + 'E';
                strcpy(courser_name_fail_message, temp.data());
				udp_single_send(courser_name_fail_message, &client_addr);
				udp_recv_message.clear();
                continue;
			}

            int index = category_selection(course_category);

            if(index == - 1){
                string temp =  "Didn't find the category: " + course_category;
                cout<<temp<<"."<<endl;
				char course_category_fail_message[1024];
                temp = temp + 'E';
                strcpy(course_category_fail_message, temp.data());
				udp_single_send(course_category_fail_message, &client_addr);
				udp_recv_message.clear();
                continue;
            }
            
            vector<string> vec = courses_map[course_name];
            string temp =  "The " + course_category + " of " + course_name + " is " +  vec[index];
            char feedback[1024];

            cout<<"The course information has been found: " + temp + "."<<endl;

            temp = temp + 'E';
            strcpy(feedback, temp.data());
        	udp_single_send(feedback, &client_addr);

            cout<<"The ServerEE finished sending the response to the Main Server."<<endl;

			udp_recv_message.clear();
		}
	}

    return 0;
}

int category_selection(string course_category){
    int index = 0;

    if(course_category == "Credit"){
        index = Credit;
    }
    else if(course_category == "Professor"){
        index = Professor;
    }
    else if(course_category == "Days"){
        index = Days;
    }
    else if(course_category == "CourseName"){
        index = CourseName;
    }
    else index = - 1;

    return index;
}

//code cited
void readTxt(string file_path){
    ifstream infile;
    infile.open(file_path.data());

    if(!infile.is_open()){
        cout<<"Can not open this file"<<endl;
    }

    string s;

    while(getline(infile, s)){
        split(s);
    }

    infile.close();
}

void split(string file_strs){
    int i = 0;
	for(i = 0; i < file_strs.size(); i++){
		if(file_strs[i] == ','){
			course_name = file_strs.substr(0, i);
            break;
		}
	}

    int start = i + 1;
    for(i = start; i < file_strs.size(); i++){
        if(file_strs[i] == ','){
            string temp = file_strs.substr(start, i - start);   
            courses_map[course_name].push_back(temp);
            start = i + 1;
        }
    }

    courses_map[course_name].push_back(file_strs.substr(start, file_strs.size() - 1));		
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
