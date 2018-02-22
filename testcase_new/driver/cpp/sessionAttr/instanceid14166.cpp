/**************************************************************
 * @Description : test case of sessionAttr
 * seqDB-14166  : 设置会话访问属性指定实例为instanceid，
 *                其中instanceid包含【8/9/10】
 * seqDB-14167  : 设置会话访问属性，
 *                指定instanceid和timeout属性
 * seqDB-14168  : 设置会话访问属性，单值指定访问实例为M/S/A
 * seqDB-14169  : 设置会话访问属性指定多个instanceid，
 *                其中节点选择模式为顺序选取
 * seqDB-14170  : getSessionAttr()获取驱动端缓存信息
 * seqDB-14171  : 设置会话访问属性指定实例为instanceid和[M/S/A]
 * seqDB-14172  : 设置timeout值，执行多次不同类型操作超时
 * seqDB-14173  : 设置timeout值，执行lob操作超时
 * @Modify      : Liang xuewang
 *                2018-02-12
 **************************************************************/
#include <client.hpp>
#include <gtest/gtest.h>
#include <stdlib.h>
#include <unistd.h>
#include <map>
#include <string>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;

#define CHECK_TIMEOUT( rc, msg ) \
do{ \
   if( SDB_TIMEOUT == rc ) \
   { \
      cout << msg << endl ; \
      goto timeout ; \
   } \
}while( 0 ) ;

class sessionAttrTest14166 : public testBase
{
protected:
   const CHAR* rgName ;
   const CHAR* csName ;
   const CHAR* clName ;
   sdbReplicaGroup rg ;
   sdbNode master ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   map<string, INT32> nodeInfo ;  // nodename instanceid
   INT32 primaryInstanceid ;

   void SetUp()
   {
      testBase::SetUp() ;
   }
   void TearDown()
   {
      testBase::TearDown() ;
   }

   INT32 init()
   {
      INT32 rc = SDB_OK ;
      rgName = "sessionAttrTestRg14166" ;
      csName = "sessionAttrTestCs14166" ;
      clName = "sessionAttrTestCl14166" ;
      BSONObj clOption ;
      const INT32 instanceids[] = { 8, 9, 10 } ;
      const INT32 size = sizeof(instanceids) / sizeof(instanceids[0]) ;

      // get local host name
      CHAR nodeHost[ MAX_NAME_SIZE+1 ] = { 0 } ;
      rc = getLocalHost( nodeHost, MAX_NAME_SIZE ) ;
      CHECK_RC( SDB_OK, rc, "fail to get local hostname" ) ; 
      
      // create rg and node with instanceid, start rg
      rc = db.createReplicaGroup( rgName, rg ) ;
      CHECK_RC( SDB_OK, rc, "fail to create rg %s", rgName ) ;
      for( INT32 i = 0;i < size;i++ )
      {
         INT32 instanceid = instanceids[i] ;
         CHAR nodeSvc[ MAX_NAME_SIZE+1 ] = { 0 } ;
         INT32 port = atoi( ARGS->rsrvPortBegin() ) + 10 * i ;
         sprintf( nodeSvc, "%d", port ) ;
         CHAR nodePath[ MAX_NAME_SIZE+1 ] = { 0 } ;
         sprintf( nodePath, "%s%s%s", ARGS->rsrvNodeDir(), "data/", nodeSvc ) ;
         BSONObj nodeOption = BSON( "instanceid" << instanceid ) ;
         string nodename = nodeHost ;
         nodename += ":" ;
         nodename += nodeSvc ;         

         cout << "create node: " << nodename << " " << nodePath 
              << " instanceid: " << instanceid << endl ;
         rc = rg.createNode( nodeHost, nodeSvc, nodePath, nodeOption ) ;
         CHECK_RC( SDB_OK, rc, "fail to create node" ) ;
         nodeInfo.insert( pair<string, INT32>( nodename, instanceid ) ) ;
      }
      rc = rg.start() ;
      CHECK_RC( SDB_OK, rc, "fail to start rg %s", rgName ) ;

      // get master node instanceid
      do
      {
         rc = rg.getMaster( master ) ;
         sleep( 1 ) ;
      }while( rc != SDB_OK ) ;
      primaryInstanceid = nodeInfo[ master.getNodeName() ] ;
      cout << "primary node instanceid: " << primaryInstanceid << endl ;

      // create cs and cl in rg
      rc = db.createCollectionSpace( csName, SDB_PAGESIZE_4K, cs ) ;
      CHECK_RC( SDB_OK, rc, "fail to create cs %s", csName ) ;
      clOption = BSON( "Group" << rgName << "ReplSize" << 0 ) ;
      rc = cs.createCollection( clName, clOption, cl ) ;
      CHECK_RC( SDB_OK, rc, "fail to create cl %s", clName ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 fini()
   {
      INT32 rc = SDB_OK ;
      rc = db.dropCollectionSpace( csName ) ;
      CHECK_RC( SDB_OK, rc, "fail to drop cs %s", csName ) ;
      rc = db.removeReplicaGroup( rgName ) ;
      CHECK_RC( SDB_OK, rc, "fail to remove rg %s", rgName ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 getExplainNode( sdbCollection& cl, const BSONObj& cond, 
                         const BSONObj& option, string& nodename )
   {
      INT32 rc = SDB_OK ;
      sdbCursor cursor ;
      BSONObj obj ;

      rc = cl.explain( cursor, cond, _sdbStaticObject, _sdbStaticObject, 
                       _sdbStaticObject, 0, -1, 0, option ) ;
      CHECK_RC( SDB_OK, rc, "fail to explain" ) ;
      rc = cursor.next( obj ) ;
      CHECK_RC( SDB_OK, rc, "fail to next" ) ;
      nodename = obj.getField( "NodeName" ).String() ;
      rc = cursor.close() ;
      CHECK_RC( SDB_OK, rc, "fail to close cursor" ) ;
   done:
      return rc ;
   error:
      goto done ;
   }
} ;

TEST_F( sessionAttrTest14166, instanceid )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }

   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   BSONObj option = BSON( "PreferedInstance" << BSON_ARRAY( 8 << 9 << 11 ) ) ;
   rc = db.setSessionAttr( option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to setSessionAttr" ;
   string nodename ;
   rc = getExplainNode( cl, _sdbStaticObject, _sdbStaticObject, nodename ) ;
   ASSERT_EQ( SDB_OK, rc ) ;  
   INT32 instanceid = nodeInfo.at( nodename ) ;
   cout << "instanceid: " << instanceid << endl ;
   ASSERT_TRUE( ( instanceid == 8 ) || ( instanceid == 9 ) ) << "fail to check instanceid 8 or 9" ;
 
   rc = fini() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( sessionAttrTest14166, timeout )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }

   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   const INT32 docNum = 50000 ;
   vector<BSONObj> docs ;
   for( INT32 i = 0;i < docNum;i++ )
   {
      docs.push_back( BSON( "a" << i ) ) ;
   }
   rc = cl.bulkInsert( 0, docs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert docs" ;
   
   // setSessionAttr, timeout 1000ms = 1s
   BSONObj option = BSON( "PreferedInstance" << 9 << "Timeout" << 1000 ) ;
   rc = db.setSessionAttr( option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to setSessionAttr" ;

   BSONObj cond = BSON( "a" << BSON( "$gt" << 1 ) ) ;
   BSONObj explainOption = BSON( "Run" << true ) ;
   string nodename ;
   rc = getExplainNode( cl, cond, explainOption, nodename ) ;
   cout << "explain rc: " << rc << endl ;
   ASSERT_TRUE( ( SDB_OK == rc ) || ( SDB_TIMEOUT == rc ) ) << "fail to check explain, rc = " << rc ;
   if( SDB_OK == rc )
   {
      cout << nodename << endl ;
      INT32 instanceid = nodeInfo.at( nodename ) ;
      ASSERT_EQ( 9, instanceid ) << "fail to check instanceid" ;
      BSONObj result ;
      rc = db.getSessionAttr( result ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to getSessionAttr" ;
      ASSERT_EQ( 9, result.getField( "PreferedInstance" ).Int() ) 
                 << "fail to check getSessionAttr PreferedInstance" ;
      ASSERT_EQ( "random", result.getField( "PreferedInstanceMode" ).String() )
                 << "fail to check getSessionAttr PreferedInstanceMode" ;
      ASSERT_EQ( 1000, result.getField( "Timeout" ).Long() ) 
                 << "fail to check getSessionAttr Timeout" ;
   }
   else
   {
      db.disconnect() ;
      rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   } 

   rc = fini() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( sessionAttrTest14166, msa )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   const CHAR* attrs[] = { "M", "S", "A", "m", "s", "a" } ;
   INT32 size = sizeof(attrs) / sizeof(attrs[0]) ;
   for( INT32 i = 0;i < size;i++ )
   {
      const CHAR* attr = attrs[i] ;
      BSONObj option = BSON( "PreferedInstance" << attr ) ;
      rc = db.setSessionAttr( option ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to sessionAttr" ;
      string nodename ;
      rc = getExplainNode( cl, _sdbStaticObject, _sdbStaticObject, nodename ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      
      cout << "attr: " << attr << " node: " << nodename << endl ;
      INT32 instanceid = nodeInfo.at( nodename ) ;
      if( !strcmp( attr, "M" ) || !strcmp( attr, "m" ) )
      {
         ASSERT_EQ( primaryInstanceid, instanceid ) << "fail to check instanceid" ;
      }
      else if( !strcmp( attr, "S" ) || !strcmp( attr, "s" ) )
      {
         ASSERT_NE( primaryInstanceid, instanceid ) << "fail to check instanceid" ;
      }
      else
      {
         ASSERT_TRUE( ( instanceid == 8 ) || ( instanceid == 9 ) ||
                      ( instanceid == 10 ) ) << "fail to check instanceid" ;
      }
   }

   rc = fini() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( sessionAttrTest14166, ordered )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   BSONObj option = BSON( "PreferedInstance" << BSON_ARRAY( 10 << 9 ) << 
                          "PreferedInstanceMode" << "ordered" ) ;
   rc = db.setSessionAttr( option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to setSessionAttr" ;
   string nodename ;
   rc = getExplainNode( cl, _sdbStaticObject, _sdbStaticObject, nodename ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   INT32 instanceid = nodeInfo.at( nodename ) ;
   ASSERT_EQ( 10, instanceid ) << "fail to check instanceid" ;
   
   BSONObj result ;
   rc = db.getSessionAttr( result ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getSessionAttr" ;
   vector<BSONElement> instances = result.getField( "PreferedInstance" ).Array() ;
   ASSERT_EQ( 2, instances.size() ) << "fail to check getSessionAttr instances size" ;
   ASSERT_EQ( 10, instances[0].Int() ) << "fail to check getSessionAttr instances[0]" ;
   ASSERT_EQ( 9, instances[1].Int() ) << "fail to check getSessionAttr instances[1]" ;
   ASSERT_EQ( "ordered", result.getField( "PreferedInstanceMode" ).String() )
              << "fail to check getSessionAttr PreferedInstanceMode" ;   
   ASSERT_EQ( -1, result.getField( "Timeout" ).Long() )
              << "fail to check getSessionAttr Timeout" ;
   
   rc = fini() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( sessionAttrTest14166, cache )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }  
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   BSONObj option = BSON( "PreferedInstance" << 10 ) ;
   rc = db.setSessionAttr( option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to setSessionAttr" ;
   BSONObj result ;
   rc = db.getSessionAttr( result ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getSessionAttr" ;
   ASSERT_EQ( 10, result.getField( "PreferedInstance" ).Int() ) ;

   option = BSON( "PreferedInstance" << 9 ) ;
   rc = db.setSessionAttr( option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to setSessionAttr" ;
   rc = db.getSessionAttr( result ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getSessionAttr" ;
   ASSERT_EQ( 9, result.getField( "PreferedInstance" ).Int() ) ;
   rc = db.getSessionAttr( result ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getSessionAttr again" ;
   ASSERT_EQ( 9, result.getField( "PreferedInstance" ).Int() ) ;

   rc = fini() ;   
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( sessionAttrTest14166, mix )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   const CHAR* instances[][3] = {
         { "M", "", "" },
         { "m", "", "" },
         { "S", "", "" },
         { "s", "", "" },
         { "A", "", "" },
         { "a", "", "" },
         { "M", "S", "A" }   
      } ;
   INT32 size = sizeof(instances) / sizeof(instances[0]) ;

   for( INT32 i = 0;i < size;i++ )
   {
      BSONArrayBuilder arrBuilder ;
      arrBuilder.append( 8 ) ;
      arrBuilder.append( 9 ) ;
      arrBuilder.append( 10 ) ;
      for( INT32 j = 0;j < 3;j++ )
      {
         const CHAR* instance = instances[i][j] ; 
         if( strcmp( "", instance ) )
         {
            arrBuilder.append( instance ) ;
         }
      }
      BSONObj obj = arrBuilder.done() ;
      BSONObjBuilder builder ;
      builder.appendArray( "PreferedInstance", obj ) ;
      builder.append( "PreferedInstanceMode", "ordered" ) ;
      BSONObj option = builder.done() ;
      cout << "sessionAttr: " << option << endl ;

      rc = db.setSessionAttr( option ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to setSessionAttr" ;
      string nodename ;
      rc = getExplainNode( cl, _sdbStaticObject, _sdbStaticObject, nodename ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      INT32 instanceid = nodeInfo.at( nodename ) ;
      switch( i )
      {
      case 0: ASSERT_EQ( primaryInstanceid, instanceid ) ;  // M
              break ;
      case 1: ASSERT_EQ( 8, instanceid ) ; // m
              break ;      
      case 2: ASSERT_NE( primaryInstanceid, instanceid ) ; // S
              break ;
      case 3:  // s 
      case 4:  // A
      case 5:  // a
              ASSERT_EQ( 8, instanceid ) ;
              break ;
      case 6: ASSERT_EQ( primaryInstanceid, instanceid ) ; // M S A
              break ;
      default: ASSERT_EQ( 1, 0 ) << "Wrong i value " << i ;
              break ;
      }
   }
   
   rc = fini() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( sessionAttrTest14166, opTimeout )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   const INT32 docNum = 50000 ;
   vector<BSONObj> docs ;
   for( INT32 i = 0;i < docNum;i++ )
   {
      docs.push_back( BSON( "a" << i ) ) ;
   }
   rc = cl.bulkInsert( 0, docs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert docs" ;
   
   // set Timeout = 1ms
   BSONObj option = BSON( "Timeout" << 1 ) ;
   rc = db.setSessionAttr( option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to setSessionAttr" ;

   BSONObj explainOption = BSON( "Run" << true ) ;
   string nodename ;
   rc = getExplainNode( cl, _sdbStaticObject, explainOption, nodename ) ;
   cout << "explain return: " << rc << endl ;
   ASSERT_TRUE( ( SDB_OK == rc ) || ( SDB_TIMEOUT == rc ) ) ;
   if( SDB_TIMEOUT == rc )
   {
      db.disconnect() ;
      rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to connect" ;
   }
   
   option = BSON( "Timeout" << -1 ) ;
   rc = db.setSessionAttr( option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to setSessionAttr Timeout -1" ;

   rc = fini() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( sessionAttrTest14166, lobTimeout )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbLob lob ;
   const INT32 lobSize = 16*1024*1024 ;
   CHAR* writeBuf = (CHAR*)malloc( lobSize * sizeof( CHAR ) ) ;
   ASSERT_TRUE( writeBuf ) << "malloc 16M writeBuf failed" ;
   memset( writeBuf, 'x', lobSize ) ;

   BSONObj option = BSON( "Timeout" << 1 ) ;
   rc = db.setSessionAttr( option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to setSessionAttr" ;
   
   rc = cl.createLob( lob ) ;
   ASSERT_TRUE( ( SDB_OK == rc ) || ( SDB_TIMEOUT == rc ) ) 
                << "fail to check rc: " << rc ; 
   CHECK_TIMEOUT( rc, "Timeout when createLob" ) ;

   rc = lob.write( writeBuf, lobSize ) ;
   ASSERT_TRUE( ( SDB_OK == rc ) || ( SDB_TIMEOUT == rc ) )
                << "fail to check rc: " << rc ;
   CHECK_TIMEOUT( rc, "Timeout when writeLob" ) ;

   rc = lob.close() ;
   ASSERT_TRUE( ( SDB_OK == rc ) || ( SDB_TIMEOUT == rc ) )
                << "fail to check rc: " << rc ;
   CHECK_TIMEOUT( rc, "Timeout when closeLob" ) ;

done:
   free( writeBuf ) ;
   option = BSON( "Timeout" << -1 ) ;
   rc = db.setSessionAttr( option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to setSessionAttr Timeout -1" ;
   rc = fini() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   return ;
timeout:
   db.disconnect() ;
   rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   goto done ;
}
