/**************************************************************************
 * @Description:   test case for C driver
 *                 seqDB-17941: sdbCreateUsr2接口支持用户级审计日志
 *                 seqDB-17957: sdbCreateUsr2接口option参数校验
 * @Modify:        liuxiaoxuan Init
 *                 2019-06-19
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include "testcommon.hpp"

class createUsr17941_17957: public testing::Test
{
protected:
   sdbConnectionHandle db ;
   sdbConnectionHandle cataDB ;
   sdbReplicaGroupHandle rg ;
   sdbNodeHandle masterNode ;
   const char* userName ;
   const char* passwd ;

   void SetUp()
   {
      INT32 rc = SDB_OK ;
      userName = "user_17941_17957" ;
      passwd = "passwd_17941_17957" ;

      getConf() ;
      rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
      ASSERT_EQ( SDB_OK, rc ) ; 

      if( isStandalone( db ) )
      {
         printf( "Run mode is standalone\n" ) ;
         return ;
      }

      // get catalog master node connection
      int groupId = 1 ;
      rc = sdbGetReplicaGroup1( db, groupId, &rg ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbGetNodeMaster( rg, &masterNode ) ;
      ASSERT_EQ( SDB_OK, rc ) ;

      const char* hostName ;
      const char* svcName ;
      const char* nodeName ;
      int nodeId = -1;
      rc = sdbGetNodeAddr( masterNode, &hostName, &svcName, &nodeName, &nodeId ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbConnect( hostName, svcName, USER, PASSWD, &cataDB ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   void TearDown()
   {
      INT32 rc = SDB_OK ;

      if( isStandalone( db ) )
      {
         printf( "Run mode is standalone\n" ) ;
         return ;
      }

      rc = sdbRemoveUsr( db, userName, passwd ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      sdbReleaseNode( masterNode ) ;
      sdbReleaseReplicaGroup( rg ) ;
      sdbDisconnect( db ) ;
      sdbDisconnect( cataDB ) ;
      sdbReleaseConnection( db ) ;
      sdbReleaseConnection( cataDB ) ;
   }
} ;

TEST_F( createUsr17941_17957, withOption )
{
   int rc = SDB_OK ;

   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }

   // invalid parameter
   bson obj ;
   bson_init( &obj ) ;
   bson_append_string( &obj, "AuditMask", "test" ) ;
   bson_finish( &obj ) ;
   rc = sdbCreateUsr2( db, userName, passwd, &obj ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   bson_destroy( &obj ) ;

   const char* mask = "CLUSTER|DML|DDL|DCL|!INSERT|UPDATE" ;
   bson_init( &obj ) ;
   bson_append_string( &obj, "AuditMask", mask ) ;
   bson_finish( &obj ) ;
   rc = sdbCreateUsr2( db, userName, passwd, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create user" ;
   bson_destroy( &obj ) ;

   sdbCollectionHandle cl ;
   rc = sdbGetCollection( cataDB, "SYSAUTH.SYSUSRS", &cl ) ;
   ASSERT_EQ( rc, SDB_OK ) << "fail to get collection SYSAUTH.SYSUSRS" ;

   CHAR destMask[64] ;
   bson subobj ;
   bson_init( &obj ) ;
   bson_init( &subobj ) ;
   sdbCursorHandle cursor ;
   rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( rc, SDB_OK ) << "fail to query" ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   bson_iterator it, sub ;
   bson_find( &it, &obj, "Options" ) ;
   bson_iterator_subobject( &it, &subobj ) ;
   bson_find( &sub, &subobj, "AuditMask" ) ;
   bson_print( &subobj ) ;
   string srcMask = bson_iterator_string( &sub ) ;
   strcpy( destMask, srcMask.c_str() ) ;
   ASSERT_EQ( 0, strcmp( destMask, mask ) ) << "check auditmask wrong, expect:" << mask << " actual:" << destMask ;
   bson_destroy( &obj ) ;
   bson_destroy( &subobj ) ;
   sdbReleaseCursor( cursor ) ;
   sdbReleaseCollection( cl ) ;
}
