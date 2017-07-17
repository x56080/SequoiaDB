#ifndef TESTCOMMON_HPP__
#define TESTCOMMON_HPP__
#include "client.h"
#include <stdio.h>
#include <vector>
#include <string>
#include <stdarg.h>

using namespace std ;

#define USER                  ""
#define PASSWD                "" 
#define RESTPORT               11814

#define CHECK_RC( rc, fmt, ... ) \
do { \
	if( rc != SDB_OK ) \
	{ \
	  printMsg( fmt, ##__VA_ARGS__ ) ; \
	  goto error ; \
	} \
} while( 0 ) ;

extern char HOSTNAME[100] ;
extern char SVCNAME[100] ;
extern char CHANGEDPREFIX[100] ;
extern char RSRVPORTBEGIN[100] ;
extern char RSRVPORTEND[100] ;
extern char RSRVNODEDIR[100] ;
extern char WORKDIR[100] ;
extern char IPADDR[100] ;
extern char HOST[100] ;

SDB_EXTERN_C_START

void printMsg( const char* fmt, ... ) ;

// create collection
int createNormalCl( sdbConnectionHandle* db, sdbCSHandle* cs, sdbCollectionHandle* cl,
				const char* csname, const char* clname ) ;

// check standalone
bool isStandalone( sdbConnectionHandle db );

// get CI parameter HOSTNAME/SVCNAME/CHANGEDPREFIX/RSRVPORTBEGIN/RSRVPORTEND/RSRVNODEDIR/WORKDIR
void getConf() ;

// get a unique name: CHANGEDPREFIX + pid + modName = name
void getUniqueName(const char* modName,char name[]) ;

// get localhost ip address like 192.168.31.61
void getLocalIpAddr() ;

// get hostname like sdbserver1
void getHost() ;

// get idle port between RSRVPORTBEGIN and RSRVPORTEND
void getIdlePort( char* port ) ;

// get all data groups
int getGroups( sdbConnectionHandle db, vector<string>& vec ) ;

// get all group nodes
int getGroupNodes( sdbConnectionHandle db, const CHAR* groupname, vector<string>& vec ) ;

SDB_EXTERN_C_END

#endif
