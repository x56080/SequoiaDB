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
#include <stdarg.h>
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

/*****************************************************************
* Print err msg
*
*****************************************************************/
void printMsg( const char* fmt, ... )
{
    va_list ap ;
    va_start( ap, fmt ) ;
    vprintf( fmt, ap ) ;
    va_end( ap ) ;
}

/*****************************************************************
* create normal collection with csname clname
* return SDB_OK if success, return others if error
*
******************************************************************/
int createNormalCl( sdbConnectionHandle* db, sdbCSHandle* cs, sdbCollectionHandle* cl,
                	  const char* csname, const char* clname )
{
	int rc = SDB_OK ;
	// connect sdb
 	getConf() ;
    rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, db ) ;
    CHECK_RC( rc, "fail to connect sdb, rc = %d\n", rc ) ;
	// create cs
	rc = sdbCreateCollectionSpace( *db, csname, SDB_PAGESIZE_4K, cs ) ;
    CHECK_RC( rc, "fail to create cs %s, rc = %d\n", csname, rc ) ;
	// create cl
	rc = sdbCreateCollection( *cs, clname, cl ) ;
    CHECK_RC( rc, "fail to create cl %s, rc = %d\n", clname, rc ) ;	
done:
	return rc ;
error:
	goto done ;
}

/*************************************************************
* check db is standalone or not, 
* if standalone return true, otherwise return false
*
*************************************************************/
bool isStandalone( sdbConnectionHandle db )
{
   bool ret = false ;
   int rc = SDB_OK ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;

   rc = sdbListReplicaGroups( db, &cursor ) ;
   if( SDB_RTN_COORD_ONLY == rc )
   {
      ret = true ;
   }
   sdbReleaseCursor(cursor) ;
   return ret ;
}

/****************************************************************
* get args like HOSTNAME SVCNAME ....
*
****************************************************************/
void getConf()
{
	static bool inited = false ;
	if( inited )
	{
		return ;
	} 
	else
	{
		inited = true ;
	}

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
    
    printf( "HOSTNAME: %s\n", HOSTNAME ) ;
    printf( "SVCNAME: %s\n", SVCNAME ) ;
    printf( "CHANGEDPREFIX: %s\n", CHANGEDPREFIX ) ;
	printf( "RSPVPORTBEGIN: %s\n", RSRVPORTBEGIN ) ;
	printf( "RSPVPORTEND: %s\n", RSRVPORTEND ) ;
	printf( "RSPVNODEDIR: %s\n", RSRVNODEDIR ) ;
	printf( "WORKDIR: %s\n", WORKDIR ) ;
    printf( "\n" ) ;
}

/****************************************************************
* get a unique name with pid, make sure name length is enough!!
*
****************************************************************/
void getUniqueName( const char* modName, char name[] )
{
   pid_t pid = getpid() ;
   sprintf( name, "%s_%d_%s", CHANGEDPREFIX, pid, modName ) ;
}

/***************************************************************
* get local ip address
*
***************************************************************/
void getLocalIpAddr()
{
	int sock ;
    struct sockaddr_in sin ;
    struct ifreq ifr ;

    sock = socket( AF_INET, SOCK_DGRAM, 0 ) ;
    if( sock == -1 )
	{
        printf( "Error in socket.\n" ) ;
		exit(1) ;
	}

    strncpy( ifr.ifr_name, ETH_NAME, IFNAMSIZ ) ;
    ifr.ifr_name[ IFNAMSIZ-1 ] = 0 ;

    if( ioctl( sock, SIOCGIFADDR, &ifr ) < 0)
    {
        printf( "Error in ioctl.\n" ) ;
		exit(1) ;
    }

    memcpy( &sin, &ifr.ifr_addr, sizeof(sin) ) ;
    sprintf( IPADDR, "%s", inet_ntoa(sin.sin_addr) ) ;
	printf( "Local Ip Address: %s\n", IPADDR ) ;
	close( sock ) ;
}

/**********************************************************
* get local hostname 
*
**********************************************************/
void getHost()
{
	int rc = gethostname( HOST, sizeof(HOST)-1 ) ;
	if( rc != 0 )
	{
	   printf( "fail to gethostname, rc = %d\n", rc ) ;
	   exit(1) ;
	}
	printf( "Host: %s\n", HOST ) ;
}

/*******************************************************************************
* get a idle port between RSRVPORTBEGIN and RSRVPORTEND in localhost
*
*******************************************************************************/
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

/***********************************************************************************
* get nodes of a group to a vector
* vector ex [ "sdbserver1:20100", "sdbserver2:20100", ... ]
* return SDB_OK if success, return others if error
*
***********************************************************************************/
int getGroupNodes( sdbConnectionHandle db, const char* groupname,
                     vector<string>& vec )
{
    int rc = SDB_OK ;
    sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
    bson cond, obj ;
    bson_init( &cond ) ;
    bson_init( &obj ) ;

    rc = bson_append_string( &cond, "GroupName", groupname ) ;
    CHECK_RC( rc, "bson fail to append GroupName:%s, rc = %d\n", groupname, rc ) ;
    rc = bson_finish( &cond ) ;
    CHECK_RC( rc, "bson fail to finish, rc = %d\n", rc ) ;
    rc = sdbGetList( db, SDB_LIST_GROUPS, &cond, NULL, NULL, &cursor ) ;
    CHECK_RC( rc, "fail to get list groups, rc = %d\n", rc ) ;

    rc = sdbNext( cursor, &obj ) ;
    CHECK_RC( rc, "fail to get sdb next, rc = %d\n", rc ) ;
    bson_iterator it, sub ;
    bson_find( &it, &obj, "Group" ) ;
    bson_iterator_subiterator( &it, &sub ) ;
    bson_iterator_next( &sub ) ;
    while( bson_iterator_more( &sub ) )
    {
        bson tmp1 ;
        bson_init( &tmp1 ) ;
        bson_iterator_subobject( &sub, &tmp1 ) ;
		bson_iterator i1 ;
        bson_find( &i1, &tmp1, "HostName" ) ;
        string hostname = bson_iterator_string( &i1 ) ;

        bson_find( &i1, &tmp1, "Service" ) ;
        bson tmp2 ;
        bson_init( &tmp2 ) ;
        bson_iterator_subobject( &i1, &tmp2 ) ;
        bson_iterator i2 ;
        bson_iterator_init( &i2, &tmp2 ) ;
        bson_iterator_next( &i2 ) ;
        bson tmp3 ;
        bson_init( &tmp3 ) ;
        bson_iterator_subobject( &i2, &tmp3 ) ;
        bson_iterator i3 ;
        bson_find( &i3, &tmp3, "Name" ) ;
        string svcname = bson_iterator_string( &i3 ) ;

        string nodename = hostname + ":" + svcname ;
        // cout << nodename << endl ;
        vec.push_back( nodename ) ;

        bson_destroy( &tmp3 ) ;
        bson_destroy( &tmp2 ) ;
        bson_destroy( &tmp1 ) ;
        bson_iterator_next( &sub ) ;
    }

done:
    bson_destroy( &cond ) ;
    bson_destroy( &obj ) ;
    sdbReleaseCursor( cursor ) ;
    return rc ;
error:
    goto done ;
}

/*****************************************************************
* get all data groups to a vector, 
* vector ex [ "group1", "group2", ... ]
* return SDB_OK if success, return others if error
*
*****************************************************************/
int getGroups( sdbConnectionHandle db, vector<string>& vec )
{
    int rc = SDB_OK ;
    sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
    bson obj ;
    bson_init( &obj ) ;

    rc = sdbListReplicaGroups( db, &cursor ) ;
    CHECK_RC( rc, "fail to list replica groups, rc = %d\n", rc ) ;
    while( !sdbNext( cursor, &obj ) )
    {
        bson_iterator it ;
        bson_find( &it, &obj, "GroupName" ) ;
        string groupname = bson_iterator_string( &it ) ;
        if( groupname != "SYSCoord" && groupname != "SYSCatalogGroup" )
        {
            // cout << groupname << endl ;
            vec.push_back( groupname ) ;
        }
        bson_destroy( &obj ) ;
        bson_init( &obj ) ;
    }

done:
    bson_destroy( &obj ) ;
	sdbReleaseCursor( cursor ) ;
    return rc ;
error:
    goto done ;
}
