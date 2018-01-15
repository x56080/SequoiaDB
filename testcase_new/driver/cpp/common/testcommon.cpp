/**************************************************************
 * @Description: common functions of driver test
 * @Modify     : Liang xuewang 
 *               2017-09-17
 ***************************************************************/
#include "testcommon.hpp"
#include <vector>
#include <iostream>
#include <unistd.h>

using namespace std;
using namespace sdbclient;
using namespace bson ;

void printMsg( const CHAR* fmt, ... )
{
    va_list ap ;
    va_start( ap, fmt ) ;
    vprintf( fmt, ap ) ;
    va_end( ap ) ;
}

void ossSleep( INT32 milliSeconds )
{
   #if defined (_WINDOWS)
      Sleep( milliSeconds ) ;
   #else
      usleep( milliSeconds * 1000 ) ;
   #endif
}

BOOLEAN isStandalone( sdb& db )
{
   sdbCursor cursor ;
   INT32 rc = db.getList( cursor, SDB_LIST_GROUPS ) ;
   if( rc == SDB_RTN_COORD_ONLY )
      return TRUE ;
   else
      return FALSE ;
}

INT32 createNormalCsCl( sdb& db, 
                        sdbCollectionSpace& cs, 
                        sdbCollection& cl,
                        const CHAR* csName, 
                        const CHAR* clName )
{
   INT32 rc = SDB_OK ;
   BSONObj option ;
   rc = db.createCollectionSpace( csName, SDB_PAGESIZE_4K, cs ) ;
   CHECK_RC( SDB_OK, rc, "fail to create cs %s", csName ) ;
   option = BSON( "ReplSize" << 0 ) ;
   rc = cs.createCollection( clName, option, cl ) ;
   CHECK_RC( SDB_OK, rc, "fail to create cl %s", clName ) ;
done:
   return rc ;
error:
   goto done ;
}

/****************************************************************************
* get a cl groups to a vector, use cl full name
* vector ex [ "group1", "group2", ... ]
* return SDB_OK if success, return others if error
*
****************************************************************************/
INT32 getClGroups( sdb& db, 
                   const CHAR* clFullName, 
                   vector<string>& clGroups )
{
   INT32 rc = SDB_OK ;
   sdbCursor cursor ;
   BSONObj obj ;
   BSONObj cond ;
   vector<BSONElement> vec ;

   cond = BSON( "Name" << clFullName ) ;
   rc = db.getSnapshot( cursor, SDB_SNAP_COLLECTIONS, cond ) ;
   CHECK_RC( SDB_OK, rc, "fail to get snapshot cl" ) ;

   rc = cursor.next( obj ) ;
   CHECK_RC( SDB_OK, rc, "fail to get next" ) ;
   vec = obj.getField( "Details" ).Array() ;
   for( INT32 i = 0;i < vec.size();i++ )
   {
      string rgName = vec[i].Obj().getField( "GroupName" ).String() ;
      clGroups.push_back( rgName ) ;
   }
   rc = cursor.close() ;
   CHECK_RC( SDB_OK, rc, "fail to close cursor" ) ;

done:
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
INT32 getGroups( sdb& db, vector<string>& groups )
{
    INT32 rc = SDB_OK ;
    sdbCursor cursor ;
    BSONObj obj ;

    rc = db.listReplicaGroups( cursor ) ;
    CHECK_RC( SDB_OK, rc, "fail to list replica groups" ) ;
    while( !cursor.next( obj ) )
    {
        string rgName = obj.getField( "GroupName" ).String() ;
        if( rgName != "SYSCoord" && rgName != "SYSCatalogGroup" )
        {
            groups.push_back( rgName ) ;
        }
    }
   rc = cursor.close() ;
   CHECK_RC( SDB_OK, rc, "fail to close cursor" ) ;

done:
    return rc ;
error:
    goto done ;
}

/***********************************************************************************
 * get nodes of a group to a vector
 * vector ex [ "sdbserver1:20100", "sdbserver2:20100", ... ]
 * return SDB_OK if success, return others if error
 *
 ***********************************************************************************/
INT32 getGroupNodes( sdb& db, 
                     const CHAR* rgName, 
                     vector<string>& nodes )
{
   INT32 rc = SDB_OK ;
   sdbCursor cursor ;
   BSONObj cond ;
   BSONObj obj ;
   vector<BSONElement> info ;

   cond = BSON( "GroupName" << rgName ) ;
   rc = db.getList( cursor, SDB_LIST_GROUPS, cond ) ;
   CHECK_RC( SDB_OK, rc, "fail to list groups" ) ;

   rc = cursor.next( obj ) ;
   CHECK_RC( SDB_OK, rc, "fail to get next" ) ;
   
   info = obj.getField( "Group" ).Array() ;
   for( INT32 i = 0;i < info.size();i++ )
   {
      BSONObj tmp = info[i].Obj() ;
      string hostName = tmp.getField( "HostName" ).String() ;
      vector<BSONElement> services = tmp.getField( "Service" ).Array() ;
      string svcName = services[0].Obj().getField( "Name" ).String() ;
      string nodeName = hostName + ":" + svcName ;
      nodes.push_back( nodeName ) ;    
   }

   rc = cursor.close() ;
   CHECK_RC( SDB_OK, rc, "fail to close cursor" ) ;

done:
   return rc ;
error:
   goto done ;
}

/********************************************************************
 * get local hostName 
 * return 0 if success, return errno if fail
 *
 ********************************************************************/
INT32 getLocalHost( CHAR hostName[], INT32 len )
{
   INT32 rc = 0 ;
   rc = gethostname( hostName, len ) ;
   CHECK_RC( 0, rc, "fail to get local hostname" ) ;
   cout << "local hostname: " << hostName << endl ;
done:
   return rc ;
error:
   goto done ;
}
