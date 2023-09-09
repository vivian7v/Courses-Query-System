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
#include <time.h>
#include <thread>
#include <pthread.h>
#include <cstring>
using namespace std;

//global variable
#define serverC_PortNumber 21770
#define serverEE_PortNumber 22770
#define serverCS_PortNumber 23770
#define serverM_UDP_PortNumber 24770
#define serverM_TCP_PortNumber 25770
#define BUFF_MAXSIZE 1024
#define IP_Address "127.0.0.1"
char IP[] = IP_Address;
char TCP_RecvMessage[1024];
char TCP_SendMessage[1024];
int Tcp_socketfd, children_sock, Udp_socketfd;
string tcp_recv_message = "";
string udp_recv_message = "";
string username = "";
string password = "";

//function declearance
void udp_recv(int serverPortNUmber);
void udp_respond(int socketfd);
void udp_requ(int socketfd, const struct sockaddr_in *addr, unsigned int len);
void udp_send(int portNumber, char message[]);
void TCP_Server(int portNumber);
void tcp_recv(int socketfd);
string encrypt(string message);
void tcp_single_send(char message[]);
void udp_client(int client_portNumber, int server_portNumber);


int main(void){
    
    cout<<"The main server is up and running."<<endl;

    thread thread_tcp(TCP_Server, serverM_TCP_PortNumber);
    thread_tcp.detach();

    thread thread_udp_listen(udp_recv, serverM_UDP_PortNumber);
    thread_udp_listen.detach();

    //Dealing with ID Authentication
    while(1){
        while(1){
            if(tcp_recv_message != ""){

                for(int i = 0; i < tcp_recv_message.size(); i++){
                    if(tcp_recv_message[i] == ','){
                        username = tcp_recv_message.substr(0, i);
                    }
                }

                cout<<"The main server received the authentication for "<<username<<" using TCP over port "<<serverM_TCP_PortNumber<<"."<<endl;
                char encrypt_username_and_password[100];
                string temp = tcp_recv_message;
                
                strcpy(encrypt_username_and_password, encrypt(temp).data());
                udp_send(serverC_PortNumber, encrypt_username_and_password);

                cout<<"The main server sent an authentication request to serverC."<<endl;
                break;
            }
        }
        
        while(1){
            if(udp_recv_message != ""){
                cout<<"The main server received the result of the authentication request from ServerC using UDP over port "<<serverM_UDP_PortNumber<<"."<<endl;
                strcpy(TCP_SendMessage, udp_recv_message.data());
                tcp_single_send(TCP_SendMessage);
                cout<<"The main server sent the authentication result to the client."<<endl;
                break;
            }
        }

        if(udp_recv_message == "ID Pass"){
            break;
        }

        udp_recv_message.clear();
        tcp_recv_message.clear();
    }


    udp_recv_message.clear();
    tcp_recv_message.clear();

    //Dealing with Query
    while(1){
        while(1){
            if(tcp_recv_message != ""){
                char query[1024];
                string temp = tcp_recv_message;
                
                string course_name = "";
                string course_category = "";
                for(int i = 0; i < tcp_recv_message.size(); i++){
				    if(tcp_recv_message[i] == ','){
					    course_name = tcp_recv_message.substr(0, i);
					    course_category = tcp_recv_message.substr(i + 1);
				    }
			    }
                
                cout<<"The main server received from "<<username<<" to query course "<<course_name<<" about "<<course_category<<" using TCP over port "<<serverM_TCP_PortNumber<<"." <<endl;

                strcpy(query, temp.data());
                if(temp[0] == 'E'){
                    udp_send(serverEE_PortNumber, query);
                    cout<<"The main server sent a request to serverEE."<<endl;
                }
                else if(temp[0] == 'C'){
                    udp_send(serverCS_PortNumber, query);
                    cout<<"The main server sent a request to serverCS."<<endl;
                }

                break;
            }
        }
        
        while(1){
            if(udp_recv_message != ""){
                if(udp_recv_message[udp_recv_message.size() - 1] == 'E'){
                    cout<<"The main server received the response from serverEE using UDP over port "<<serverM_UDP_PortNumber<<"."<<endl;
                }
                else if(udp_recv_message[udp_recv_message.size() - 1] == 'C'){
                    cout<<"The main server received the response from serverCS using UDP over port "<<serverM_UDP_PortNumber<<"."<<endl;
                }
                udp_recv_message.pop_back();
                strcpy(TCP_SendMessage, udp_recv_message.data());
                tcp_single_send(TCP_SendMessage);
                cout<<"The main server sent the query information to the client."<<endl;
                break;
            }
        }

        udp_recv_message.clear();
        tcp_recv_message.clear();
    }


    return 0;
}

void udp_respond(int socketfd){
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

void udp_recv(int serverPortNUmber){
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
    server_addr.sin_port = htons(serverPortNUmber);

    if(bind(socketfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in)) < 0){
        cout<<"Bind Error"<<endl;
        exit(1);
    }

    udp_respond(socketfd);
    close(socketfd);
}

void udp_requ(char message[], int socketfd, const struct sockaddr_in *addr, unsigned int len){
	sendto(socketfd, message, strlen(message), 0, (struct sockaddr*)addr, len);
}

void udp_send(int portNumber, char message[]){
	int socketfd;
	struct sockaddr_in severM_addr;

	socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(socketfd == -1){
		cout<<"Socket Error"<<endl;
		exit(1);
	}

	bzero(&severM_addr, sizeof(struct sockaddr_in));
	severM_addr.sin_family = AF_INET;
	severM_addr.sin_port = htons(portNumber);
	if(inet_aton(IP, &severM_addr.sin_addr) == -1){
		cout<<"Ip Error"<<endl;
		exit(1);
	}

	udp_requ(message, socketfd, &severM_addr, sizeof(struct sockaddr_in));

	close(socketfd);
}


void tcp_single_send(char message[]){
        if(write(children_sock, message, strlen(message)) == -1){
            cout<<"Write Error"<<endl;
            exit(1);
        }
}

void tcp_recv(int socketfd){
    while(1){
        int nbytes;
        nbytes = recv(socketfd, TCP_RecvMessage, 1024, 0);
        if(nbytes == -1){
            cout<<"Receive Error"<<endl;
            exit(1);
        }
        TCP_RecvMessage[nbytes] = '\0';
        tcp_recv_message = TCP_RecvMessage;
        if(nbytes == 0)continue;
    }

}

void TCP_Server(int portNumber){
    struct sockaddr_in serverM_addr;
    struct sockaddr_in client_addr;
    unsigned int sin_size;

    Tcp_socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(Tcp_socketfd == -1){
        cout<<"Socket Error"<<endl;
        exit(1);
    }

    bzero(&serverM_addr, sizeof(struct sockaddr_in));
    serverM_addr.sin_family = AF_INET;
    serverM_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverM_addr.sin_port = htons(portNumber);

    if(bind(Tcp_socketfd, (struct sockaddr*)(&serverM_addr), sizeof(struct sockaddr)) == -1){
        cout<<"Bind Error"<<endl;
        exit(1);
    }

    if(listen(Tcp_socketfd, 10) == -1){
        cout<<"Lisent Error"<<endl;
        exit(1);
    }

    sin_size = sizeof(struct sockaddr_in);
    children_sock = accept(Tcp_socketfd,(struct sockaddr*)(&client_addr), &sin_size);
    if(children_sock == -1){
        cout<<"Accept Error"<<endl;
        exit(1);
    }
    cout<<"Server get connection from: "<<inet_ntoa(client_addr.sin_addr)<<endl;


    tcp_recv(children_sock);

    close(children_sock);
    close(Tcp_socketfd);
}

string encrypt(string message){
    //offset
    for(int i = 0; i < message.size(); i++){
        if(message[i] >= '0' && message[i] <= '9'){
            if(message[i] + 4 > '9'){
                message[i] = '3' - ('9' - message[i]);
            } 
            else message[i] += 4;
        }
        else if(message[i] >= 'A' && message[i] <= 'Z'){
            if(message[i] + 4 > 'Z'){
                message[i] = 'D' - ('Z' - message[i]);
            }
            else message[i] += 4;
        }
        else if(message[i] >='a' && message[i] <= 'z'){
            if(message[i] + 4 > 'z'){
                message[i] = 'd' - ('z' - message[i]);
            }
            else message[i] +=4;
        }
    }

    return message;
}

void udp_client(int client_portNumber, int server_portNumber){
    struct sockaddr_in udp_server_addr;
    struct sockaddr_in udp_client_addr;

	Udp_socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(Udp_socketfd == -1){
		cout<<"Socket Error"<<endl;
		exit(1);
	}

	bzero(&udp_server_addr, sizeof(sockaddr_in));
	bzero(&udp_client_addr, sizeof(sockaddr_in));

    udp_server_addr.sin_family = AF_INET;
    udp_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    udp_server_addr.sin_port = htons(server_portNumber);

	udp_client_addr.sin_family = AF_INET;
	udp_client_addr.sin_port = htons(client_portNumber);


    if(bind(Udp_socketfd, (struct sockaddr*)&udp_server_addr, sizeof(struct sockaddr_in)) < 0){
        cout<<"Bind Error"<<endl;
        exit(1);
    }

	if(inet_aton(IP, &udp_client_addr.sin_addr) == -1){
		cout<<"Ip Error"<<endl;
		exit(1);
	}

	// thread thread_recv(upd_respond, Udp_socketfd);
	// thread_recv.detach();
    udp_respond(Udp_socketfd);

	//udp_requ(Udp_socketfd, &client_addr, sizeof(struct sockaddr_in));


	close(Udp_socketfd);
}





