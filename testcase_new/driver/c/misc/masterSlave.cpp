/**************************************************************************
 * @Description:   test case for C driver
 *                 seqDB-13637:获取数据组主节点/从节点
 *                 SEQUOIADBMAINSTREAM-2871
 * @Modify:        Liang xuewang Init
 *                 2017-11-28
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <unistd.h>
#include "testcommon.hpp"

class masterSlaveTest : public testing::Test
{
protected:
   const CHAR* rgName ;
   sdbReplicaGroupHandle rg ;
   sdbConnectionHandle db ;

   void SetUp()
   {
      INT32 rc = SDB_OK ;
      getConf() ;
      rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb" ;
   }

   void TearDown()
   {
      sdbDisconnect( db ) ;
      sdbReleaseConnection( db ) ;
   }

   INT32 init()
   {
      INT32 rc = SDB_OK ;
      rgName = "masterSlaveTestRg" ; 
      rc = sdbCreateReplicaGroup( db, rgName, &rg ) ;
      CHECK_RC( rc, "fail to create rg %s", rgName ) ;
   done:
      return rc ;
   error:
      goto done ;
   }
   INT32 fini()
   {
      INT32 rc = SDB_OK ;
      rc = sdbRemoveReplicaGroup( db, rgName ) ;
      CHECK_RC( rc, "fail to remove rg %s", rgName ) ;
      sdbReleaseReplicaGroup( rg ) ;
   done:
      return rc ;
   error:
      goto done ;
   }  
} ;

TEST_F( masterSlaveTest, masterSlave )
{
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone, can't create group\n" ) ;
      return ;
   }
   
   INT32 rc = SDB_OK ;
   rc = init() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbNodeHandle master ;
   sdbNodeHandle slave ;

   // getMaster and getSlave in a empty group
   rc = sdbGetNodeMaster( rg, &master ) ;
   ASSERT_EQ( SDB_CLS_EMPTY_GROUP, rc ) << "fail to test getMaster in empty group" ;
   rc = sdbGetNodeSlave( rg, &slave ) ;
   ASSERT_EQ( SDB_CLS_EMPTY_GROUP, rc ) << "fail to test getSlave in empty group" ;

   // create two node in rg and start
   getHost() ;
   const CHAR* svcName1 = RSRVPORTBEGIN ;
   const CHAR* svcName2 = RSRVPORTEND ;
   const CHAR* nodeDir = RSRVNODEDIR ;
   CHAR dbPath1[100] ;
   sprintf( dbPath1, "%s%s%s%s", nodeDir, "data/", svcName1, "/" ) ;
   CHAR dbPath2[100] ;
   sprintf( dbPath2, "%s%s%s%s", nodeDir, "data/", svcName2, "/" ) ;
   cout << "node1: " << HOST << " " << svcName1 << " " << dbPath1 << endl ;
   cout << "node2: " << HOST << " " << svcName2 << " " << dbPath2 << endl ;

   rc = sdbCreateNode( rg, HOST, svcName1, dbPath1, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create node1" ;
   rc = sdbCreateNode( rg, HOST, svcName2, dbPath2, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create node2" ;
   rc = sdbStartReplicaGroup( rg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to start rg " << rgName ;

   // getMaster and getSlave in a normal group
   rc = sdbGetNodeMaster( rg, &master ) ;
   while( SDB_RTN_NO_PRIMARY_FOUND == rc )
   {
      usleep( 1000 * 1000 ) ;  // sleep 1 sec
      rc = sdbGetNodeMaster( rg, &master ) ;
   }
   ASSERT_EQ( SDB_OK, rc ) << "fail to getMaster" ;
   rc = sdbGetNodeSlave( rg, &slave ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getSlave" ;   

   // stop slave, master will be step down to slave, no master in group
   // then getMaster getSlave
   rc = sdbStopNode( slave ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to stop master" ;
   rc = sdbGetNodeMaster( rg, &master ) ;
   while( SDB_OK == rc )
   {
      usleep( 1000 * 1000 ) ;
      rc = sdbGetNodeMaster( rg, &master ) ;
   }
   ASSERT_EQ( SDB_RTN_NO_PRIMARY_FOUND, rc ) << "fail to test getMaster" ; 
   rc = sdbGetNodeSlave( rg, &slave ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to test getSlave" ;

   // stop rg
   rc = sdbStopReplicaGroup( rg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to stop rg " << rgName ;

   sdbReleaseNode( master ) ;
   sdbReleaseNode( slave ) ;   

   rc = fini() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}
