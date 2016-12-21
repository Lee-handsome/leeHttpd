#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
using namespace std;

#define PORT 9999
#define MAX_BUF 1024*8

void* accept_request(void* clientfd);
void do_get(char* url,int client);
void do_post(char* url,int client);
void send_staticfile(int client, char* path);
void startup_cgi();
void response404(int client);
void send_header(int client);

void* accept_request(void* clientfd)
{

	int client=(int)*((int*)clientfd);
	cout<<"accept:"<<client<<endl;
	char buf[MAX_BUF];
	char* method;
	char* url;

	recv(client,buf,sizeof(buf)-1,0);
	cout<<endl;
	cout<<"request:"<<buf<<endl;
	method = strtok(buf," ");
	cout<<"method:"<<method<<endl;
	url = strtok(NULL," ");
	cout<<endl;
	cout<<"url:"<<url<<endl;

	if(strcmp(method,"GET")==0){
		do_get(url,client);
	}
	else if(strcmp(method,"POST")==0){
		do_post(url,client);
	}
	else response404(client);
	pthread_exit(NULL);
}

void do_get(char* url,int client)
{
	char* ptr=url;
	char path[512];
	bool cgi=0;
	int i=0;
	while(*ptr!='\0') {
		if(*ptr=='?'){
			cgi=1;
			break;
		}
		path[i++]=*ptr;
		ptr++;
	}
	path[i]='\0';
	if(path[strlen(path)-1]=='/'){
		strcat(path,"/index.html");
	}

	if(cgi){
		startup_cgi();
	}
	else{
		send_staticfile(client,path);
	}
	close(client);
}

void do_post(char* url,int client)
{

}
void send_staticfile(int client, char* path)
{
	FILE* fp;
	//strcat(".",path);
	char pathname[512];
	sprintf(pathname, "www/%s", path);
	fp=fopen(pathname,"rb");
	char buf[1024];
	if(fp==NULL){
		perror("open staticfile error");
		response404(client);
		return;
	}
	send_header(client);
	fgets(buf, sizeof(buf), fp);
    while (!feof(fp))
    {
    	
        send(client, buf, strlen(buf), 0); 
        fgets(buf, sizeof(buf), fp);
    }
    close(client);
}

void startup_cgi()
{

}
void response404(int client)
{
	send_header(client);
	char body[] = "<html><body>404 Not Found from leeHttpd</body></html>";
	send(client,body,strlen(body),0);
	//close(client);
}

void send_header(int client)
{
	char status[] = "HTTP/1.0 200 OK\r\n";
    char header[] = "Server: leeHttpd\r\nContent-Type: text/html\r\n\r\n";
    send(client,status,strlen(status),0);
    send(client,header,strlen(header),0);
}

int main()
{
	int server_sock;
	int client_sock;
	pthread_t tid;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	server_sock = socket(PF_INET,SOCK_STREAM,0);
	//assert(server_sock != -1);
	if(server_sock == -1){
		perror("socket error");
		exit(1);
	}

	//init server_addr
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT);
	if(-1 == bind(server_sock,(struct sockaddr*)&server_addr,sizeof(server_addr))){
		perror("bind error");
		exit(1);
	}

	if(-1 == listen(server_sock,5)){
		perror("listen error");
		exit(1);
	}

	while(1){
		client_sock = accept(server_sock,(struct sockaddr*)&client_addr,&client_len);
		if(client_sock == -1){
			perror("accept error");
			exit(1);
		}
		if (pthread_create(&tid , NULL, accept_request, &client_sock) != 0)
            perror("pthread_create error");
	}
	close(server_sock);
	return 0;
}