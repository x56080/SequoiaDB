/*****************************************************************************
*@Description : 
*@Modify List : 2016-9-6  Ting YU  Init
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <gtest/gtest.h>
#include <string.h>
#include "client.h"
#include "../common/testcommon.hpp"
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFLEN 1024

void checkServer()
{
   int rc;
   // connect to coord
   sdbConnectionHandle db = 0;
   getConf() ;
   rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db );
   ASSERT_GE(rc, 0) << "connect coord error";
}

int initClient()
{
   int rc;
   int sockfd;
   
   // create socket  
   sockfd = socket(AF_INET, SOCK_STREAM, 0);    
   
   // get addr
   char *hostip = "127.0.0.1";
   struct sockaddr_in addr;
   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_port = htons(RESTPORT);
   addr.sin_addr.s_addr = inet_addr(hostip);
   
   // connect     
   rc = connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr));   
   if (rc == -1)
	{
	   perror("connect error");
	   return rc ;   
	}
   
   return(sockfd);
}

TEST( restAbnormal, multi_send )
{   
   int rc;
   int sockfd;
   sockfd = initClient();
   if(sockfd == -1) return ; 
   
   // send message
   char restStr[8];
   sprintf(restStr, "%d", RESTPORT);
   char *hostip = "127.0.0.1";
   
   char sendbuf[BUFLEN];
   memset(sendbuf, 0, sizeof(sendbuf));
   strcpy(sendbuf, "POST / HTTP/1.1\r\n"); 
   strcat(sendbuf, "Content-Type: application/x-www-form-urlencoded;charset=UTF-8\r\n");
   strcat(sendbuf, "Content-Length: 22\r\n");
   strcat(sendbuf, "Host: ");
   strcat(sendbuf, hostip);
   strcat(sendbuf, ":");
   strcat(sendbuf, restStr);
   strcat(sendbuf, "\r\n");
   strcat(sendbuf, "\r\ncmd=list");
   
   rc = send(sockfd, sendbuf, strlen(sendbuf), 0);
   ASSERT_GE(rc, 0) << "first send error";
   
   char sendbuf2[BUFLEN]; 
   memset(sendbuf2, 0, sizeof(sendbuf));
   strcat(sendbuf2, "%20collections");
   
   rc = send(sockfd, sendbuf2, strlen(sendbuf2), 0);
   ASSERT_GE(rc, 0) << "second send error";
   
   // recv and check returned errno 
   char recvbuf[BUFLEN];
   char* totalrecvbuf = (char*)malloc( sizeof(char) * BUFLEN );
   if( !totalrecvbuf )
   {
        printf( "Error malloc space for totalrecvbuf.\n" );
        return;
   }
   int totalSize = BUFLEN ;
   int availSize = BUFLEN ;
   int pos = 0;
   while( (rc = recv(sockfd, recvbuf, BUFLEN, 0)) > 0 )
   {
       if( rc > availSize )
       {
           char* tmp = (char*)realloc( totalrecvbuf, sizeof(char) * (totalSize + BUFLEN) ) ;
           if( !tmp )
           {
               printf( "Error realloc space for totalrecvbuf.\n" ) ;
               break ;
           }
           totalrecvbuf = tmp ;
           totalSize += BUFLEN ;
           availSize += BUFLEN ;
        }
        memcpy( totalrecvbuf + pos, recvbuf, rc ) ;
        availSize -= rc ;
        pos += rc ;
   }
   printf("%s\n", totalrecvbuf);
   char *p = strstr(totalrecvbuf, "\r\n\r\n{ \"errno\": 0 }");
   ASSERT_STRNE(NULL, p) << "check recieve message error";
   free( totalrecvbuf ) ;

   close(sockfd); 
   
   checkServer();      
}

TEST( restAbnormal, post_lack_terminator )
{   
   int rc;
   int sockfd;
   sockfd = initClient();
   if(sockfd == -1) return ; 
   
   // send message
   char restStr[8];
   sprintf(restStr, "%d", RESTPORT);
   char *hostip = "127.0.0.1";
   char sendbuf[BUFLEN];   
   memset(sendbuf, 0, sizeof(sendbuf));
   strcpy(sendbuf, "POST / HTTP/1.1\r\n"); 
   strcat(sendbuf, "Content-Type: application/x-www-form-urlencoded;charset=UTF-8\r\n");
   strcat(sendbuf, "Content-Length: 28\r\n"); //length is 22 
   strcat(sendbuf, "Host: ");
   strcat(sendbuf, hostip);
   strcat(sendbuf, ":");
   strcat(sendbuf, restStr);
   strcat(sendbuf, "\r\n");
   strcat(sendbuf, "\r\ncmd=list%20collections");
   rc = send(sockfd, sendbuf, strlen(sendbuf), 0);
   ASSERT_GE(rc, 0) << "send error";
   
   // recv 
   char recvbuf[BUFLEN];
   char* totalrecvbuf = (char*)malloc( sizeof(char) * BUFLEN );
   if( !totalrecvbuf )
   {
        printf( "Error malloc space for totalrecvbuf.\n" );
        return;
   }
   int totalSize = BUFLEN ;
   int availSize = BUFLEN ;
   int pos = 0;
   while( (rc = recv(sockfd, recvbuf, BUFLEN, 0)) > 0 )
   {
       if( rc > availSize )
       {
           char* tmp = (char*)realloc( totalrecvbuf, sizeof(char) * (totalSize + BUFLEN) ) ;
           if( !tmp )
           {
               printf( "Error realloc space for totalrecvbuf.\n" ) ;
               break ;
           }
           totalrecvbuf = tmp ;
           totalSize += BUFLEN ;
           availSize += BUFLEN ;
        }
        memcpy( totalrecvbuf + pos, recvbuf, rc ) ;
        availSize -= rc ;
        pos += rc ;
   }
   printf("%s\n", totalrecvbuf);
   free( totalrecvbuf ) ;
      
   close(sockfd);    
   
   checkServer();      
}

TEST( restAbnormal, get_lack_terminator )
{   
   int rc;
   int sockfd;
   sockfd = initClient();
   if(sockfd == -1) return ; 
   
   // send message
   char restStr[8];
   sprintf(restStr, "%d", RESTPORT);
   char *hostip = "127.0.0.1";
   char sendbuf[BUFLEN];
   memset(sendbuf, 0, sizeof(sendbuf));
   strcpy(sendbuf, "GET /?cmd=list%20collections HTTP/1.1\r\n");
   strcat(sendbuf, "Host: ");
   strcat(sendbuf, hostip);
   strcat(sendbuf, ":");
   strcat(sendbuf, restStr);
   strcat(sendbuf, "\r\n"); // lack of end "\r\n\r\n"
   rc = send(sockfd, sendbuf, strlen(sendbuf), 0);
   ASSERT_GE(rc, 0) << "send error";
   
  // recv 
   char recvbuf[BUFLEN];
   char* totalrecvbuf = (char*)malloc( sizeof(char) * BUFLEN );
   if( !totalrecvbuf )
   {
        printf( "Error malloc space for totalrecvbuf.\n" );
        return;
   }
   int totalSize = BUFLEN ;
   int availSize = BUFLEN ;
   int pos = 0;
   while( (rc = recv(sockfd, recvbuf, BUFLEN, 0)) > 0 )
   {
       if( rc > availSize )
       {
           char* tmp = (char*)realloc( totalrecvbuf, sizeof(char) * (totalSize + BUFLEN) ) ;
           if( !tmp )
           {
               printf( "Error realloc space for totalrecvbuf.\n" ) ;
               break ;
           }
           totalrecvbuf = tmp ;
           totalSize += BUFLEN ;
           availSize += BUFLEN ;
        }
        memcpy( totalrecvbuf + pos, recvbuf, rc ) ;
        availSize -= rc ;
        pos += rc ;
   }
   printf("%s\n", totalrecvbuf);
   free( totalrecvbuf ) ;
      
   close(sockfd);    
   
   checkServer();      
}

TEST( restAbnormal, format_not_match_protocol1 )
{   
   int rc;
   int sockfd;
   sockfd = initClient();
   if(sockfd == -1) return ; 
   
   // send message   
   char sendbuf[BUFLEN];
   memset(sendbuf, 0, sizeof(sendbuf));
   strcpy(sendbuf, "\r\n\r\n");  
   rc = send(sockfd, sendbuf, strlen(sendbuf), 0);
   ASSERT_GE(rc, 0) << "send error";
   
   // recv  
   char recvbuf[BUFLEN];
   char* totalrecvbuf = (char*)malloc( sizeof(char) * BUFLEN );
   if( !totalrecvbuf )
   {
        printf( "Error malloc space for totalrecvbuf.\n" );
        return;
   }
   int totalSize = BUFLEN ;
   int availSize = BUFLEN ;
   int pos = 0;
   while( (rc = recv(sockfd, recvbuf, BUFLEN, 0)) > 0 )
   {
       if( rc > availSize )
       {
           char* tmp = (char*)realloc( totalrecvbuf, sizeof(char) * (totalSize + BUFLEN) ) ;
           if( !tmp )
           {
               printf( "Error realloc space for totalrecvbuf.\n" ) ;
               break ;
           }
           totalrecvbuf = tmp ;
           totalSize += BUFLEN ;
           availSize += BUFLEN ;
        }
        memcpy( totalrecvbuf + pos, recvbuf, rc ) ;
        availSize -= rc ;
        pos += rc ;
   }
   printf("%s\n", totalrecvbuf);
   free( totalrecvbuf ) ;
      
   close(sockfd);    
   
   checkServer();      
}

TEST( restAbnormal, format_not_match_protocol2 )
{   
   int rc;
   int sockfd;
   sockfd = initClient(); 
   if(sockfd == -1) return ;
   
   // send message   
   char sendbuf[BUFLEN];
   memset(sendbuf, 0, sizeof(sendbuf));
   strcpy(sendbuf, "GE123gaetyt4a4tyawy49");  
   rc = send(sockfd, sendbuf, strlen(sendbuf), 0);
   ASSERT_GE(rc, 0) << "send error";
   
   // recv
   char recvbuf[BUFLEN];
   char* totalrecvbuf = (char*)malloc( sizeof(char) * BUFLEN );
   if( !totalrecvbuf )
   {
        printf( "Error malloc space for totalrecvbuf.\n" );
        return;
   }
   int totalSize = BUFLEN ;
   int availSize = BUFLEN ;
   int pos = 0;
   while( (rc = recv(sockfd, recvbuf, BUFLEN, 0)) > 0 )
   {
       if( rc > availSize )
       {
           char* tmp = (char*)realloc( totalrecvbuf, sizeof(char) * (totalSize + BUFLEN) ) ;
           if( !tmp )
           {
               printf( "Error realloc space for totalrecvbuf.\n" ) ;
               break ;
           }
           totalrecvbuf = tmp ;
           totalSize += BUFLEN ;
           availSize += BUFLEN ;
        }
        memcpy( totalrecvbuf + pos, recvbuf, rc ) ;
        availSize -= rc ;
        pos += rc ;
   }
   printf("%s\n", totalrecvbuf);
   free( totalrecvbuf ) ;
      
   close(sockfd);    
   
   checkServer();      
}

