#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
using namespace std;

#define PORT 8888
#define MAX_BUF 1024*8

void* accept_request(void* clientfd);
void do_get(char* url,int client);
void do_post(char* url,int client,char* content);
void send_staticfile(int client, char* path);
void startup_cgi(int client,char* method,char* path,char* param,char* content);
void response404(int client);
void send_header(int client);

void* accept_request(void* clientfd)
{
	int client=(int)*((int*)clientfd);
	//cout<<"accept:"<<client<<endl;
	char request[MAX_BUF];
	char* content;
	char* line;
	char* method;
	char* url;

	recv(client,request,sizeof(request),0);
	cout<<endl;
	cout<<"accept:"<<client<<"request:"<<request<<endl;
	char* ptr= strtok(request,"\r\n");
	line = ptr;
	while(ptr!=NULL){
		content=ptr;
		ptr=strtok(NULL,"\r\n");
	}
	
	method=strtok(line," ");
	cout<<"method:"<<method<<endl;
	url = strtok(NULL," ");
	cout<<endl;
	cout<<"url:"<<url<<endl;

	if(strcmp(method,"GET")==0){
		do_get(url,client);
	}
	else if(strcmp(method,"POST")==0){
		cout<<"content:"<<content<<endl;
		do_post(url,client,content);
	}
	else response404(client);
	pthread_exit(NULL);
}

void do_get(char* url,int client)
{
	char* ptr=url;
	char path[512];
	char* param;
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
	cout<<"path"<<path<<endl;
	param = ptr;
	//cout<<"param:"<<param<<endl;

	if(path[strlen(path)-1]=='/'){
		strcat(path,"/index.html");
	}

	if(cgi){
		startup_cgi(client,"GET",path,param,NULL);
	}
	else{
		send_staticfile(client,path);
	}
	close(client);
}

void do_post(char* url,int client,char* content)
{
	char path[512];
	startup_cgi(client,"POST",url,NULL,content);
	cerr<<"post close"<<endl;
	close(client);

}
void send_staticfile(int client, char* path)
{
	FILE* fp;
	//strcat(".",path);
	char pathname[512];
	sprintf(pathname, "www%s", path);
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
    cout<<"@@@@@@@@@"<<"duwan"<<endl;
    fclose(fp);
    //close(client);
}

void startup_cgi(int client,char* method,char* path,char* param,char* content)
{
	int cgi_in[2];
	int cgi_out[2];
	pid_t pid;
	char status[] = "HTTP/1.0 200 OK\r\n";
	send(client,status,strlen(status),0);

	if(-1==pipe(cgi_in)){
		perror("pipe error");
		return;
	}
	if(-1==pipe(cgi_out)){
		perror("pipe error");
		return;
	}
	if ((pid = fork()) < 0 ) {
        perror("fork error");
        return;
    }
    if (pid == 0)
    {
    	cout<<"i am cgi"<<endl;
    	char pathname[512];
		sprintf(pathname,"./www%s",path);
		cout<<"pathname:"<<pathname<<endl;
    	//cout<<"path"<<path<<endl;
        char method_env[512];
        char query_env[512];
        char length_env[512];

        dup2(cgi_out[1], 1);
        dup2(cgi_in[0], 0);

        close(cgi_out[0]);
        close(cgi_in[1]);

        sprintf(method_env, "REQUEST_METHOD=%s", method);
        putenv(method_env);

        if (strcmp(method, "GET") == 0) {
            sprintf(query_env, "QUERY_STRING=%s", param);
            putenv(query_env);
        }
        else 
        {
        	int content_length = strlen(content);
   			sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
            putenv(length_env);
        }
        //
        cerr<<pathname<<endl;
        execl(pathname, pathname, NULL);
        perror("exec error");
        response404(client);
        exit(0);
    } 
    else 
    {    /* parent */
        close(cgi_out[1]);
        close(cgi_in[0]);

        char buf[1024];
        //char content[1024];
        if (strcmp(method, "POST") == 0)
        {
        	//cerr<<"buf:"<<buf<<endl;
        	write(cgi_in[1],content,strlen(content)+1);
        }
        
            //for (i = 0; i < content_length; i++) {
            //    recv(client, content, 1, 0);
            //    write(cgi_input[1], content, 1);
            //}

        //send_header(client);
        while (read(cgi_out[0], buf, sizeof(buf)) > 0){
        	cerr<<"buf:"<<buf<<endl;
            send(client, buf, sizeof(buf), 0);
        }

        close(cgi_out[0]);
        close(cgi_in[1]);
        waitpid(pid, NULL, 0);
    }
}

void response404(int client)
{
	send_header(client);
	char body[] = "<html><body>404 Not Found from leeHttpd</body></html>";
	send(client,body,strlen(body),0);
	close(client);
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
	int opt = 1;
	if(-1 == setsockopt(server_sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt)))
	{
		perror("setsockopt error");
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

	if(-1 == listen(server_sock,1024)){
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