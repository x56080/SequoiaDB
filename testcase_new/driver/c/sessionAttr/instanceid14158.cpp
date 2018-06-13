/**************************************************************
 * @Description : test case of sessionAttr
 * seqDB-14158  : 设置会话访问属性指定实例为instanceid，
 *                其中instanceid包含【8/9/10】
 * seqDB-14159  : 设置会话访问属性，
 *                指定instanceid和timeout属性
 * seqDB-14160  : 设置会话访问属性，单值指定访问实例为M/S/A
 * seqDB-14161  : 设置会话访问属性指定多个instanceid，
 *                其中节点选择模式为顺序选取
 * seqDB-14162  : getSessionAttr()获取驱动端缓存信息
 * seqDB-14163  : 设置会话访问属性指定实例为instanceid和[M/S/A]
 * seqDB-14164  : 设置timeout值，执行多次不同类型操作超时
 * seqDB-14165  : 设置timeout值，执行lob操作超时
 * @Modify      : Liang xuewang
 *                2018-02-12
 **************************************************************/
#include <client.h>
#include <gtest/gtest.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <map>
#include <string>
#include "testcommon.hpp"

void ossSleep(UINT32 milliseconds)
{
#if defined (_WINDOWS)
   Sleep(milliseconds);
#else
   usleep(milliseconds*1000);
#endif
}
#define CHECK_TIMEOUT( rc, msg ) \
do{ \
   if( SDB_TIMEOUT == rc ) \
   { \
      printf( "%s\n", msg ) ; \
      goto timeout ; \
   } \
}while( 0 ) ;

class sessionAttrTest14158 : public testing::Test
{
protected:
   const CHAR* host ;
   const CHAR* svc ;
   const CHAR* user ;
   const CHAR* passwd ; 
   const CHAR* rgName ;
   const CHAR* csName ;
   const CHAR* clName ;
   sdbConnectionHandle db ;
   sdbReplicaGroupHandle rg ;
   sdbNodeHandle master ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   map<string, INT32> nodeInfo ;  // nodename instanceid
   INT32 primaryInstanceid ;

   void SetUp()
   {
      getConf() ;
      INT32 rc = SDB_OK ;
      host = HOSTNAME ;
      svc = SVCNAME ;
      user = USER ;
      passwd = PASSWD ;
      rc = sdbConnect( host, svc, user, passwd, &db ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to connect" ;
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      sdbDisconnect( db ) ;
      sdbReleaseConnection( db ) ;
   }

   INT32 init()
   {
      INT32 rc = SDB_OK ;
      rgName = "sessionAttrTestRg14158" ;
      csName = "sessionAttrTestCs14158" ;
      clName = "sessionAttrTestCl14158" ;
      bson clOption ;
      const CHAR* masterNodeName ;
      
      const INT32 instanceids[] = { 8, 9, 10 } ;
      const INT32 size = sizeof(instanceids) / sizeof(instanceids[0]) ;

      // get local host name
      getHost() ;
      const CHAR* nodeHost = HOST ;
      
      // create rg and node with instanceid, start rg
      rc = sdbCreateReplicaGroup( db, rgName, &rg ) ;
      CHECK_RC( rc, "fail to create rg %s", rgName ) ;
      for( INT32 i = 0;i < size;i++ )
      {
         INT32 instanceid = instanceids[i] ;
         CHAR nodeSvc[ MAX_NAME_SIZE+1 ] = { 0 } ;
         INT32 port = atoi( RSRVPORTBEGIN ) + 10 * i ;
         sprintf( nodeSvc, "%d", port ) ;
         CHAR nodePath[ MAX_NAME_SIZE+1 ] = { 0 } ;
         sprintf( nodePath, "%s%s%s", RSRVNODEDIR, "data/", nodeSvc ) ;
   
         bson nodeOption ;
         bson_init( &nodeOption ) ;
         bson_append_int( &nodeOption, "instanceid", instanceid ) ;
         bson_finish( &nodeOption ) ;

         string nodename = nodeHost ;
         nodename += ":" ;
         nodename += nodeSvc ;         

         printf( "create node: %s %s, instanceid: %d\n", nodename.c_str(), nodePath, instanceid ) ;
         rc = sdbCreateNode( rg, nodeHost, nodeSvc, nodePath, &nodeOption ) ;
         bson_destroy( &nodeOption ) ;
         CHECK_RC( rc, "fail to create node" ) ;
         nodeInfo.insert( pair<string, INT32>( nodename, instanceid ) ) ;
      }
      rc = sdbStartReplicaGroup( rg ) ;
      CHECK_RC( rc, "fail to start rg %s", rgName ) ;

      // get master node instanceid
      do
      {
         rc = sdbGetNodeMaster( rg, &master ) ;
         sleep( 1 ) ;
      }while( rc != SDB_OK ) ;
      rc = sdbGetNodeAddr( master, NULL, NULL, &masterNodeName, NULL ) ;
      CHECK_RC( rc, "fail to get master node addr" ) ;
      primaryInstanceid = nodeInfo.at( masterNodeName ) ;
      cout << "primary node instanceid: " << primaryInstanceid << endl ;

      // create cs and cl in rg
      rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
      CHECK_RC( rc, "fail to create cs %s", csName ) ;

      bson_init( &clOption ) ;
      bson_append_string( &clOption, "Group", rgName ) ;
      bson_append_int( &clOption, "ReplSize", 0 ) ;
      bson_finish( &clOption ) ;
      rc = sdbCreateCollection1( cs, clName, &clOption, &cl ) ;
      bson_destroy( &clOption ) ;
      CHECK_RC( rc, "fail to create cl %s", clName ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 fini()
   {
      INT32 rc = SDB_OK ;
      INT32 sleepTimeLen = 2000 ;
      INT32 alreadySleepLen = 0 ;
      do{
         rc = sdbDropCollectionSpace( db, csName ) ;
         ossSleep( 10 ) ;
         alreadySleepLen += 10 ;
         if ( alreadySleepLen > sleepTimeLen ) break ;
      }while( SDB_LOCK_FAILED == rc ) ;
      CHECK_RC( rc, "fail to drop cs %s", csName ) ;
      rc = sdbRemoveReplicaGroup( db, rgName ) ;
      CHECK_RC( rc, "fail to remove rg %s", rgName ) ;
   done:
      sdbReleaseCollection( cl ) ;
      sdbReleaseCS( cs ) ;
      sdbReleaseNode( master ) ;
      sdbReleaseReplicaGroup( rg ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 getExplainNode( sdbCollectionHandle cl, bson* cond, 
                         bson* option, string& nodename )
   {
      INT32 rc = SDB_OK ;
      sdbCursorHandle cursor ;
      bson obj ;
      bson_iterator it ;

      rc = sdbExplain( cl, cond, NULL, NULL, NULL, 0, -1, 0, option, &cursor ) ;
      CHECK_RC( rc, "fail to explain" ) ;
      bson_init( &obj ) ;
      rc = sdbNext( cursor, &obj ) ;
      CHECK_RC( rc, "fail to next" ) ;
      bson_find( &it, &obj, "NodeName" ) ;
      nodename = bson_iterator_string( &it ) ;
      bson_destroy( &obj ) ;
      rc = sdbCloseCursor( cursor ) ;
      CHECK_RC( rc, "fail to close cursor" ) ;
      sdbReleaseCursor( cursor ) ;
   done:
      return rc ;
   error:
      goto done ;
   }
} ;

TEST_F( sessionAttrTest14158, instanceid )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }

   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson option ;
   bson_init( &option ) ;
   bson_append_start_array( &option, "PreferedInstance" ) ;
   bson_append_int( &option, "0", 8 ) ;
   bson_append_int( &option, "1", 9 ) ;
   bson_append_int( &option, "2", 11 ) ;
   bson_append_finish_array( &option ) ;
   bson_finish( &option ) ;
   rc = sdbSetSessionAttr( db, &option ) ;
   bson_destroy( &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to setSessionAttr" ;

   string nodename ;
   rc = getExplainNode( cl, NULL, NULL, nodename ) ;
   ASSERT_EQ( SDB_OK, rc ) ;  
   INT32 instanceid = nodeInfo.at( nodename ) ;
   printf( "instanceid: %d\n", instanceid ) ;
   ASSERT_TRUE( ( instanceid == 8 ) || ( instanceid == 9 ) ) << "fail to check instanceid 8 or 9" ;
 
   rc = fini() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( sessionAttrTest14158, timeout )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }

   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   const INT32 docNum = 50000 ;
   bson* docs[ docNum ] ;
   for( INT32 i = 0;i < docNum;i++ )
   {
      docs[i] = bson_create() ;
      bson_append_int( docs[i], "a", i ) ;
      bson_finish( docs[i] ) ;
   }
   rc = sdbBulkInsert( cl, 0, docs, docNum ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert docs" ;
   for( INT32 i = 0;i < docNum;i++ )
   {
      bson_dispose( docs[i] ) ;
   }
   
   // setSessionAttr, timeout 1000ms = 1s
   bson option ;
   bson_init( &option ) ;
   bson_append_int( &option, "PreferedInstance", 9 ) ;
   bson_append_int( &option, "Timeout", 1000 ) ;
   bson_finish( &option ) ;
   rc = sdbSetSessionAttr( db, &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to setSessionAttr" ;

   bson cond ;
   bson_init( &cond ) ;
   bson_append_start_object( &cond, "a" ) ;
   bson_append_int( &cond, "$gt", 1 ) ;
   bson_append_finish_object( &cond ) ;
   bson_finish( &cond ) ;
   
   bson explainOption ;
   bson_init( &explainOption ) ;
   bson_append_bool( &explainOption, "Run", true ) ;
   bson_finish( &explainOption ) ;

   string nodename ;
   rc = getExplainNode( cl, &cond, &explainOption, nodename ) ;
   bson_destroy( &cond ) ;
   bson_destroy( &explainOption ) ;
   printf( "explain rc: %d\n", rc ) ;
   ASSERT_TRUE( ( SDB_OK == rc ) || ( SDB_TIMEOUT == rc ) ) << "fail to check explain, rc = " << rc ;
   if( SDB_OK == rc )
   {
      printf( "%s\n", nodename.c_str() ) ;
      INT32 instanceid = nodeInfo.at( nodename ) ;
      ASSERT_EQ( 9, instanceid ) << "fail to check instanceid" ;
      bson result ;
      bson_init( &result ) ;
      rc = sdbGetSessionAttr( db, &result ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to getSessionAttr" ;

      bson_iterator it ;
      bson_find( &it, &result, "PreferedInstance" ) ;
      ASSERT_EQ( 9, bson_iterator_int( &it ) ) 
                 << "fail to check getSessionAttr PreferedInstance" ;
      bson_find( &it, &result, "PreferedInstanceMode" ) ;
      ASSERT_STREQ( "random", bson_iterator_string( &it ) ) 
                 << "fail to check getSessionAttr PreferedInstanceMode" ;
      bson_find( &it, &result, "Timeout" ) ;
      ASSERT_EQ( 1000, bson_iterator_long( &it ) ) 
                 << "fail to check getSessionAttr Timeout" ;
      bson_destroy( &result ) ;
   }
   else
   {
      sdbDisconnect( db ) ;
      rc = sdbConnect( host, svc, user, passwd, &db ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   } 

   rc = fini() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( sessionAttrTest14158, msa )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   const CHAR* attrs[] = { "M", "S", "A", "m", "s", "a" } ;
   INT32 size = sizeof(attrs) / sizeof(attrs[0]) ;
   for( INT32 i = 0;i < size;i++ )
   {
      const CHAR* attr = attrs[i] ;
      bson option ;
      bson_init( &option ) ;
      bson_append_string( &option, "PreferedInstance", attr ) ;
      bson_finish( &option ) ;
      rc = sdbSetSessionAttr( db, &option ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to sessionAttr" ;
      string nodename ;
      rc = getExplainNode( cl, NULL, NULL, nodename ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      
      printf( "attr: %s, node: %s\n", attr, nodename.c_str() ) ;
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

TEST_F( sessionAttrTest14158, ordered )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson option ;
   bson_init( &option ) ;
   bson_append_start_array( &option, "PreferedInstance" ) ;
   bson_append_int( &option, "0", 10 ) ;
   bson_append_int( &option, "1", 9 ) ;
   bson_append_finish_array( &option ) ;
   bson_append_string( &option, "PreferedInstanceMode", "ordered" ) ;
   bson_finish( &option ) ;
   rc = sdbSetSessionAttr( db, &option ) ;
   bson_destroy( &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to setSessionAttr" ;
   string nodename ;
   rc = getExplainNode( cl, NULL, NULL, nodename ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   INT32 instanceid = nodeInfo.at( nodename ) ;
   ASSERT_EQ( 10, instanceid ) << "fail to check instanceid" ;
   
   bson result ;
   bson_init( &result ) ;
   rc = sdbGetSessionAttr( db, &result ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getSessionAttr" ;
   
   bson_iterator it ;
   bson_find( &it, &result, "PreferedInstance" ) ;
   bson_iterator sub ;
   bson_iterator_subiterator( &it, &sub ) ;
   bson_iterator_next( &sub ) ;
   ASSERT_EQ( 10, bson_iterator_int( &sub ) ) << "fail to check getSessionAttr PreferedInstance" ;
   bson_iterator_next( &sub ) ;
   ASSERT_EQ( 9, bson_iterator_int( &sub ) ) << "fail to check getSessionAttr PreferedInstance" ;
   ASSERT_EQ( BSON_EOO, bson_iterator_next( &sub ) ) ;

   bson_find( &it, &result, "PreferedInstanceMode" ) ;
   ASSERT_STREQ( "ordered", bson_iterator_string( &it ) )
              << "fail to check getSessionAttr PreferedInstanceMode" ;   

   bson_find( &it, &result, "Timeout" ) ;
   ASSERT_EQ( -1, bson_iterator_long( &it ) ) 
              << "fail to check getSessionAttr Timeout" ;
   bson_destroy( &result ) ;   

   rc = fini() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( sessionAttrTest14158, cache )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }  
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson option ;
   bson_init( &option ) ;
   bson_append_int( &option, "PreferedInstance", 10 ) ;
   bson_finish( &option ) ;
   rc = sdbSetSessionAttr( db, &option ) ;
   bson_destroy( &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to setSessionAttr" ;

   bson result ;
   bson_init( &result ) ;
   rc = sdbGetSessionAttr( db, &result ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getSessionAttr" ;
   bson_iterator it ;
   bson_find( &it, &result, "PreferedInstance" ) ;
   ASSERT_EQ( 10, bson_iterator_int( &it ) ) ;
   bson_destroy( &result ) ;

   bson_init( &option ) ;
   bson_append_int( &option, "PreferedInstance", 9 ) ;
   bson_finish( &option ) ;
   rc = sdbSetSessionAttr( db, &option ) ;
   bson_destroy( &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to setSessionAttr" ;
   
   bson_init( &result ) ;
   rc = sdbGetSessionAttr( db, &result ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getSessionAttr" ;
   bson_find( &it, &result, "PreferedInstance" ) ;
   ASSERT_EQ( 9, bson_iterator_int( &it ) ) ;
   bson_destroy( &result ) ;

   bson_init( &result ) ;
   rc = sdbGetSessionAttr( db, &result ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getSessionAttr again" ;
   bson_find( &it, &result, "PreferedInstance" ) ;
   ASSERT_EQ( 9, bson_iterator_int( &it ) ) ;
   bson_destroy( &result ) ;

   rc = fini() ;   
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( sessionAttrTest14158, mix )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
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
      bson option ;
      bson_init( &option ) ;
      bson_append_start_array( &option, "PreferedInstance" ) ;
      bson_append_int( &option, "0", 8 ) ;
      bson_append_int( &option, "1", 9 ) ;
      bson_append_int( &option, "2", 10 ) ;
      for( INT32 j = 0;j < 3;j++ )
      {
         const CHAR* instance = instances[i][j] ; 
         if( strcmp( "", instance ) )
         {
            CHAR key[ MAX_NAME_SIZE+1 ] = { 0 } ;
            sprintf( key, "%d", j+3 ) ;
            bson_append_string( &option, key, instance ) ;
         }
      }
      bson_append_finish_array( &option ) ;
      bson_append_string( &option, "PreferedInstanceMode", "ordered" ) ;
      bson_finish( &option ) ;
      bson_print( &option ) ;

      rc = sdbSetSessionAttr( db, &option ) ;
      bson_destroy( &option ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to setSessionAttr" ;

      string nodename ;
      rc = getExplainNode( cl, NULL, NULL, nodename ) ;
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

TEST_F( sessionAttrTest14158, opTimeout )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   const INT32 docNum = 50000 ;
   bson* docs[ docNum ] ;
   for( INT32 i = 0;i < docNum;i++ )
   {
      docs[i] = bson_create() ;
      bson_append_int( docs[i], "a", i ) ;
      bson_finish( docs[i] ) ;
   }
   rc = sdbBulkInsert( cl, 0, docs, docNum ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert docs" ;
   for( INT32 i = 0;i < docNum;i++ )
   {
      bson_dispose( docs[i] ) ;
   }
   
   // set Timeout = 1ms
   bson option ;
   bson_init( &option ) ;
   bson_append_int( &option, "Timeout", 1 ) ;
   bson_finish( &option ) ;
   rc = sdbSetSessionAttr( db, &option ) ;
   bson_destroy( &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to setSessionAttr" ;

   bson explainOption ;
   bson_init( &explainOption ) ;
   bson_append_bool( &explainOption, "Run", true ) ;
   bson_finish( &explainOption ) ;
   string nodename ;
   rc = getExplainNode( cl, NULL, &explainOption, nodename ) ;
   bson_destroy( &explainOption ) ;
   printf( "explain return: %d\n", rc ) ;
   ASSERT_TRUE( ( SDB_OK == rc ) || ( SDB_TIMEOUT == rc ) ) ;
   if( SDB_TIMEOUT == rc )
   {
      sdbDisconnect( db ) ;
      rc = sdbConnect( host, svc, user, passwd, &db ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to connect" ;
   }
   
   bson_init( &option ) ;
   bson_append_int( &option, "Timeout", -1 ) ;
   bson_finish( &option ) ;
   rc = sdbSetSessionAttr( db, &option ) ;
   bson_destroy( &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to setSessionAttr Timeout -1" ;

   rc = fini() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( sessionAttrTest14158, lobTimeout )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   sdbLobHandle lob ;
   const INT32 lobSize = 16*1024*1024 ; 
   CHAR* writeBuf = (CHAR*)malloc( lobSize * sizeof(CHAR) ) ;
   ASSERT_TRUE( writeBuf ) << "malloc 16M writeBuf failed" ;
   memset( writeBuf, 'x', lobSize ) ;

   bson option ;
   bson_init( &option ) ;
   bson_append_int( &option, "Timeout", 1 ) ;
   bson_finish( &option ) ;
   rc = sdbSetSessionAttr( db, &option ) ;
   bson_destroy( &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to setSessionAttr" ;
   
   bson_oid_t oid ;
   bson_oid_gen( &oid ) ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   ASSERT_TRUE( ( SDB_OK == rc ) || ( SDB_TIMEOUT == rc ) ) 
                << "fail to check rc: " << rc ; 
   CHECK_TIMEOUT( rc, "Timeout when createLob" ) ;

   rc = sdbWriteLob( lob, writeBuf, lobSize ) ;
   ASSERT_TRUE( ( SDB_OK == rc ) || ( SDB_TIMEOUT == rc ) )
                << "fail to check rc: " << rc ;
   CHECK_TIMEOUT( rc, "Timeout when writeLob" ) ;

   rc = sdbCloseLob( &lob ) ;
   ASSERT_TRUE( ( SDB_OK == rc ) || ( SDB_TIMEOUT == rc ) )
                << "fail to check rc: " << rc ;
   CHECK_TIMEOUT( rc, "Timeout when closeLob" ) ;

done:
   bson_init( &option ) ;
   bson_append_int( &option, "Timeout", -1 ) ;
   bson_finish( &option ) ;
   rc = sdbSetSessionAttr( db, &option ) ;
   bson_destroy( &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to setSessionAttr Timeout -1" ;
   rc = fini() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   return ;
timeout:
   sdbDisconnect( db ) ;
   rc = sdbConnect( host, svc, user, passwd, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   goto done ;
}
