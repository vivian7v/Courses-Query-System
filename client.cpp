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
#include <cstring>
using namespace std;

//global variable
#define serverM_TCP_PortNumber 25770
#define IP_Address "127.0.0.1"
char IP[] = IP_Address;
char TCP_RecvMessage[1024];
char TCP_SendMessage[1024];
string tcp_recv_message = "";
string tcp_send_message = "";
string user_name ="";
string user_passwords = "";
string course_code = "";
string course_category = "";
int socketfd;
int error_count = 0;
int client_portNumber = 0;

//function declearance
void TCP_client(int portNumber);
void tcp_recv(int socketfd);
void tcp_single_send(char message[]);


int main(){
    //Welcome Message
    cout<<"client is up and running."<<endl;

    //Open TCP
    thread thread_TCP(TCP_client, serverM_TCP_PortNumber);
    thread_TCP.detach();

    //ID Check Phase
    while(1){
        while(1){
            cout<<"Please enter the username: ";
            cin>>user_name;

            //turning higher case to lower case
            for(int i = 0; i < user_name.size(); i++){
                if(user_name[i] >= 'A' && user_name[i] <= 'Z'){
                    user_name[i] += 32;
                }
            }

            if(user_name.size() >= 5 && user_name.size() <= 50){
                user_name = user_name;
                break;
            }
            //cout<<"Error!!!: username should be lower case characters and 5~50 chars"<<endl;
        }

        while(1){
            cout<<"Please enter the password: ";
            cin>>user_passwords;

            if(user_passwords.size() >= 5 && user_passwords.size() <= 50){
                user_passwords = user_passwords;
                break;
            }
            //cout<<"Error!!!: password should be 5~50"<<endl;
        }

        string temp = user_name + ',' + user_passwords;
        strcpy(TCP_SendMessage, temp.data());
        tcp_single_send(TCP_SendMessage);
        cout<<user_name<<" sent an authentication request to the main server."<<endl;

        while(1){
            if(tcp_recv_message != ""){
                break;
            }
        }

        if(tcp_recv_message == "ID Pass"){
            cout<<user_name<<" received the result of authentication using TCP over port "<<client_portNumber<<". Authentication is successful"<<endl;
            tcp_recv_message.clear();
            break;
        }
        
        cout<<user_name<<" received the result of authentication using TCP over port "<<client_portNumber<<". Authentication failed: "<<tcp_recv_message<<endl;
        tcp_recv_message.clear();
        user_name.clear();
        user_passwords.clear();
        error_count++;//couting error input

        if(error_count == 3){
            cout<<"Authentication Failed for 3 attempts. Client will shut down."<<endl;
            exit(1);
        }
        cout<<"Attempts remaining: "<<3-error_count<<endl;
    }

    //Query Phase
    while(1){
        cout<<"Please enter the course code to query: ";
        cin>>course_code;

        cout<<"Please enter the category (Credit / Professor / Days / CourseName): ";
        cin>>course_category;

        string query = course_code + ',' + course_category;
        strcpy(TCP_SendMessage, query.data());
        tcp_single_send(TCP_SendMessage);
        cout<<user_name<<"  sent a request to the main server."<<endl;
        
        tcp_recv_message.clear();
        while(1){
            if(tcp_recv_message != ""){
                cout<<user_name<<"The client received the response from the Main server using TCP over port "<<client_portNumber<<"."<<endl;
                
                cout<<tcp_recv_message<<endl;
                break;
            }
        }
        tcp_recv_message.clear();

        cout<<endl;
        cout<<endl;
        cout<<"-----Start a new request-----"<<endl;
    }

    return 0;
}


void tcp_single_send(char message[]){
        if(write(socketfd, message, strlen(message)) == -1){
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

void TCP_client(int portNumber){
    struct sockaddr_in server_addr;
    struct hostent *server_host;

    server_host = gethostbyname(IP);
    if(server_host == NULL){
        cout<<"Get Hostname Error"<<endl;
        exit(1);
    }

    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfd == -1){
        cout<<"Socket Error"<<endl;
        exit(1);
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portNumber);
    server_addr.sin_addr = *((struct in_addr*)server_host->h_addr);

    if(connect(socketfd, (struct sockaddr*)(&server_addr), sizeof(struct sockaddr)) == -1){
        cout<<"Connect Error"<<endl;
        exit(1);
    }

    struct sockaddr_in Myaddr;
    memset(&Myaddr,0,sizeof(Myaddr));
    unsigned int len = sizeof(Myaddr);
    
    getsockname(socketfd,(sockaddr*)&Myaddr,&len);
    client_portNumber = ntohs(Myaddr.sin_port);

    tcp_recv(socketfd);
    
    close(socketfd);
}



