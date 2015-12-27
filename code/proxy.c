#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<netdb.h>
#include<errno.h>
#include "proxy_parse.h"
//#include"proxy_parse.c" //uncomment this if using with gcc compiler

pid_t pid;
struct sockaddr_in addr_in,cli_addr,serv_addr;
struct hostent* host;
int locsockfd;
socklen_t newsockfd,clilen; 
int port=80;//change this to 8080 if used for proxy server 
int next(int pid)
{
    struct sockaddr_in host_addr;
    int n,sockfd1;
    char buffer[500];
    bzero((char*)buffer,500);
    recv(newsockfd,buffer,500,0);
    int len = strlen(buffer);
    
    //parsing starts here 
    struct ParsedRequest *req = ParsedRequest_create();
    if (ParsedRequest_parse(req, buffer, len) < 0) {
       printf("parse failed\n");
       return -1;
    }

    printf("Method:%s\n", req->method);
    printf("Host:%s\n", req->host);
    printf("version:%s\n",req->version); 


    host=gethostbyname(req->host);//make req->host to "172.31.1.4" if using with proxy server
    //parsing finished
    bzero((char*)&host_addr,sizeof(host_addr));
    host_addr.sin_port=htons(port);
    host_addr.sin_family=AF_INET;
    bcopy((char*)host->h_addr,(char*)&host_addr.sin_addr.s_addr,host->h_length);
   
    sockfd1=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    connect(sockfd1,(struct sockaddr*)&host_addr,sizeof(struct sockaddr));
    sprintf(buffer,"\nConnected to %s  IP - %s\n",req->host,inet_ntoa(host_addr.sin_addr));

    bzero((char*)buffer,sizeof(buffer));
    sprintf(buffer, "%s %s %s\r\nHost: %s\r\nConnection: close\r\n\r\n",req->method,req->path,req->version,req->host);

    n=send(sockfd1,buffer,strlen(buffer),0);
     printf("\n%s\n",buffer);

     bzero((char*)buffer,500);
     n=recv(sockfd1,buffer,500,0);
     if(!(n<=0))
       send(newsockfd,buffer,n,0);
   
    while(n>0)
    {
     bzero((char*)buffer,500);
     n=recv(sockfd1,buffer,500,0);
     if(!(n<=0))
       send(newsockfd,buffer,n,0);
     }
   
   close(sockfd1);
   close(newsockfd);
   close(locsockfd);
   _exit(0);

}
   

int connection(char* argv[])
{ 
  newsockfd=accept(locsockfd,(struct sockaddr*)&cli_addr,&clilen);

  pid=fork();
  if(pid==0)
  {
    next(pid);
   }
  else
  {
   close(newsockfd); 
   connection(argv); // recursion is used after closing newsockfd socket 
  }

  return 0;
}
  
int main(int argc,char* argv[])
{
  bzero((char*)&serv_addr,sizeof(serv_addr));
  bzero((char*)&cli_addr, sizeof(cli_addr));

  serv_addr.sin_family=AF_INET;
  serv_addr.sin_port=htons(atoi(argv[1]));
  serv_addr.sin_addr.s_addr=INADDR_ANY;
  locsockfd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
  bind(locsockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
  listen(locsockfd,50);
   
  connection(argv); //sending port as argument
  
  return 0;
}
