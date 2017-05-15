#ifndef TESTCOMMON_H__
#define TESTCOMMON_H__
#include "client.h"

#define USER                  ""
#define PASSWD                "" 
#define RESTPORT               11814

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

INT32 createCollection ( sdbConnectionHandle *cl, CHAR *csName, CHAR *clName );

// check standalone
BOOLEAN isStandalone( sdbConnectionHandle db );

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

SDB_EXTERN_C_END

#endif
