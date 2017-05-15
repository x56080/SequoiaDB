#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <string.h>
#include "testcommon.hpp"

using namespace std ;
using testing::internal::String ;
using testing::internal::g_argvs ;
extern vector<String> g_argvs ;

#define ETH_NAME "eth0"

// default value
char HOSTNAME[100]      = "localhost" ;
char SVCNAME[100]       = "11810" ;
char CHANGEDPREFIX[100] = "sdv_c_test" ;
char RSRVPORTBEGIN[100] = "26000" ;
char RSRVPORTEND[100]   = "27000" ;
char RSRVNODEDIR[100]   = "/opt/sequoiadb/database/" ;
char WORKDIR[100]       = "/tmp/ctest" ; 

char IPADDR[100]        = "192.168.31.61" ;
char HOST[100]          = "sdbserver1" ;

INT32 createCollection( sdbCollectionHandle *cl,
                        CHAR *csName,
                        CHAR *clName )
{
   INT32 rc = SDB_OK;
   sdbConnectionHandle db = 0;
   sdbCSHandle cs = 0;
   bson clOptions;   

   // connect to sdb
   getConf() ;
   rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db );
   if ( rc )
   {
      printf( "Failed to connect database, hostname=%s, coord= %s\n",
              HOSTNAME, SVCNAME );     
      goto error ;
   }
   
   // create cs   
   rc = sdbCreateCollectionSpace( db, csName, 65536, &cs );
   if( SDB_DMS_CS_EXIST == rc )
   {
      rc = sdbGetCollectionSpace( db, csName, &cs );
   }
   if( rc )
   {
      printf( "Failed to get cs %s\n", csName );
      goto error ;
   }
   
   // create cl
   bson_init( &clOptions );
   bson_append_int( &clOptions, "ReplSize", 0 );
   bson_finish( &clOptions );
   
   rc = sdbDropCollection( cs, clName );
   if( SDB_DMS_NOTEXIST != rc && SDB_OK != rc )
   {
      printf( "Failed to drop cl %s.%s in ready\n", csName, clName );
   }

   rc = sdbCreateCollection1( cs, clName, &clOptions, cl );
   if( rc )
   {
      printf( "Failed to create cl %s.%s\n", csName, clName );
      goto error ;
   }
   
   bson_destroy( &clOptions );
   
done:   
   return rc;
error:
   goto done;  
}

BOOLEAN isStandalone( sdbConnectionHandle db )
{
   BOOLEAN isStdalone = FALSE;
   INT32 rc = SDB_OK;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;

   rc = sdbListReplicaGroups( db, &cursor ) ;
   if( SDB_RTN_COORD_ONLY == rc )
   {
      isStdalone = TRUE;
   }
   sdbReleaseCursor(cursor) ;
   return isStdalone;
}

void getConf()
{
    printf( "Print command args:\n" ) ;
    for( int i = 0;i < g_argvs.size();i++ )
      printf( "%s ",g_argvs[i].c_str() ) ;
    printf( "\n" ) ;
    
    for( int i = 0;i < g_argvs.size();i++ )
    {
      String para = g_argvs[i] ;
      if( para == "--hostname" || para == "-n" )
         strcpy( HOSTNAME,g_argvs[i+1].c_str() ) ;
      else if( para == "--svcname" || para == "-s" )
         strcpy( SVCNAME,g_argvs[i+1].c_str() ) ;
      else if( para == "--changedprefix" || para == "-c" )
         strcpy( CHANGEDPREFIX,g_argvs[i+1].c_str() ) ;
	  else if( para == "--rsrvportbegin" || para == "-b" )
		 strcpy( RSRVPORTBEGIN,g_argvs[i+1].c_str() ) ;
	  else if( para == "--rsrvportend" || para == "-e" )
		 strcpy( RSRVPORTEND,g_argvs[i+1].c_str() ) ;
	  else if( para == "--rsrvnodedir" || para == "-d" )
		 strcpy( RSRVNODEDIR,g_argvs[i+1].c_str() ) ;
	  else if( para == "--workdir" || para == "-w" )
		 strcpy( WORKDIR,g_argvs[i+1].c_str() ) ; 
    }
    
    printf("HOSTNAME: %s\n",HOSTNAME) ;
    printf("SVCNAME: %s\n",SVCNAME) ;
    printf("CHANGEDPREFIX: %s\n",CHANGEDPREFIX) ;
	printf("RSPVPORTBEGIN: %s\n",RSRVPORTBEGIN) ;
	printf("RSPVPORTEND: %s\n",RSRVPORTEND) ;
	printf("RSPVNODEDIR: %s\n",RSRVNODEDIR) ;
	printf("WORKDIR: %s\n",WORKDIR) ;
    printf("\n") ;
}

void getUniqueName(const char* modName,char name[])
{
   pid_t pid = getpid() ;
   sprintf( name,"%s_%d_%s",CHANGEDPREFIX,pid,modName ) ;
}

void getLocalIpAddr()
{
	int sock ;
    struct sockaddr_in sin ;
    struct ifreq ifr ;

    sock = socket( AF_INET, SOCK_DGRAM,0 ) ;
    if( sock == -1 )
	{
        printf( "Error in socket.\n" ) ;
	}

    strncpy( ifr.ifr_name, ETH_NAME,IFNAMSIZ ) ;
    ifr.ifr_name[IFNAMSIZ-1] = 0 ;

    if( ioctl( sock, SIOCGIFADDR,&ifr ) < 0)
    {
        printf( "Error in ioctl.\n" ) ;
    }

    memcpy( &sin, &ifr.ifr_addr, sizeof(sin) ) ;
    sprintf( IPADDR, "%s", inet_ntoa(sin.sin_addr) ) ;
	printf( "Local Ip Address: %s\n", IPADDR ) ;
	close( sock ) ;
}

void getHost()
{
	int rc = gethostname( HOST, sizeof(HOST)-1 ) ;
	if( rc != 0 )
	{
	   printf( "fail to gethostname, rc = %d\n", rc ) ;
	   exit( EXIT_FAILURE ) ;
	}
	printf( "Host: %s\n", HOST ) ;
}

void getIdlePort( char* port )
{
	int start = atoi( RSRVPORTBEGIN ) ;
	int end = atoi( RSRVPORTEND ) ;
	struct sockaddr_in servaddr ;
	int sock, i, serverport, ret ;
	
	bzero( &servaddr, sizeof(servaddr) ) ;
	servaddr.sin_family = AF_INET ;
	inet_pton( AF_INET, "127.0.0.1", &servaddr.sin_addr ) ;
	for( i = start; i <= end; i += 5 )
	{
		servaddr.sin_port = htons(i) ;
		ret = connect( sock, (struct sockaddr*)&servaddr, sizeof(servaddr) ) ;
		if( EISCONN == ret )  continue ;
		close( sock ) ;
		sprintf( port, "%d", i ) ;
		break ;
	}
}
