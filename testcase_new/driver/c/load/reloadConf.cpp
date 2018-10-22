/**************************************************************
 * @Description: test case for Jira questionaire Task
 *					  SEQUOIADBMAINSTREAM-2165
 *					  seqDB-11001:reloadConf
 *               修改数据组备节点的配置文件weight=20
 *					  重新选主，检查备节点升级为主节点(may not be)
 * @Modify     : Liang xuewang Init
 *			 		  2017-01-22
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include "testcommon.hpp"
#include "impWorker.hpp"

INT32 getInstallPath( CHAR* path )
{
   INT32 rc = SDB_OK ;
   const CHAR* installFile = "/etc/default/sequoiadb" ;
   FILE* fp = fopen( installFile, "r" ) ;
   CHAR s[ MAX_NAME_SIZE+1 ] = { 0 } ;
   INT32 len ;
   const CHAR* installStr = "INSTALL_DIR=" ;
   INT32 installStrLen = strlen( installStr ) ;
   if( !fp )
   {
      printf( "fail to open file /etc/default/sequoiadb\n" ) ;
      goto error ;
   }
   while( fgets( s, sizeof( s ), fp ) )
   {
      CHAR* idx = strstr( s, installStr ) ;
      if( idx )
      {
         strcpy( path, idx + installStrLen ) ;
         break ;
      }
   }
   fclose( fp ) ; 
   if( !strcmp( path, "" ) )
   {
      printf( "fail to get install path\n" ) ;
      goto error ;
   }
   len = strlen( path ) ;
   path[ len-1 ] = 0 ;  // change the last character \n to 0
done:
   return rc ;
error:
   rc = SDB_TEST_ERROR ;
   goto done ;
}

INT32 isMasterNode( sdbReplicaGroupHandle rg, const CHAR* host, const CHAR* svc, BOOLEAN* res )
{
   int rc = SDB_OK ;
   sdbNodeHandle master = SDB_INVALID_HANDLE ;
   const char *host1, *svc1 ;

   do {
      rc = sdbGetNodeMaster( rg, &master ) ;
      sleep( 1 ) ;
   } while( rc == SDB_CLS_NODE_NOT_EXIST ) ;
   CHECK_RC( rc, "fail to get master node\n" ) ;
   rc = sdbGetNodeAddr( master, &host1, &svc1, NULL, NULL ) ;
   CHECK_RC( rc, "fail to get master node addr\n" ) ;
   printf( "master node: %s:%s\n", host1, svc1 ) ;

   if( strcmp(host, host1) == 0 && strcmp(svc, svc1) == 0 )
      *res = true ;
   else
      *res = false ;

done:
   sdbReleaseNode( master ) ;
   return rc ;
error:
   goto done ;
}

// get a slave data node which is on the same machine with coord
INT32 createSlaveNode( sdbConnectionHandle db, sdbReplicaGroupHandle* rg, 
                       sdbNodeHandle* node, const CHAR** host, const CHAR** svc )
{
   int rc = SDB_OK ;
   bson obj ;
   bson_init( &obj ) ;
   char installPath[ MAX_NAME_SIZE+1 ] = { 0 } ;
   char dbpath[ MAX_NAME_SIZE+1 ] = { 0 } ;
   vector<string> groups ;
   
   // list rg
   rc = getGroups( db, groups ) ;
   CHECK_RC( rc, "fail to get groups" ) ;
   for( int i = 0;i < groups.size();i++ )
   {
      const char* rgName = groups[i].c_str() ;
      vector<string> nodes ;
      rc = getGroupNodes( db, rgName, nodes ) ;
      CHECK_RC( rc, "fail to get rg nodes" ) ;
      
      // if rg has only one node, after reelect and change primary node to new add node, 
      // then stop the primary node, group can't make reelect
      if( nodes.size() == 1 )  continue ;
        
      rc = sdbGetReplicaGroup( db, rgName, rg ) ;
      CHECK_RC( rc, "fail to get rg %s", rgName ) ;
      break ;
   }
	
   getHost() ;  // get local hostname
   getInstallPath( installPath ) ;
   sprintf( dbpath, "%s%s%s", installPath, "/database/data/", RSRVPORTBEGIN ) ;
   rc = sdbCreateNode( *rg, HOST, RSRVPORTBEGIN, dbpath, NULL ) ;
   CHECK_RC( rc, "fail to create node %s:%s dbpath: %s\n", HOST, RSRVPORTBEGIN, dbpath ) ;
   rc = sdbGetNodeByHost( *rg, HOST, RSRVPORTBEGIN, node ) ;
   CHECK_RC( rc, "fail to get node %s:%s\n", HOST, RSRVPORTBEGIN ) ;
   rc = sdbGetNodeAddr( *node, host, svc, NULL, NULL ) ;
   CHECK_RC( rc, "fail to get node addr\n" ) ;

done:
   bson_destroy( &obj ) ;
   return rc ;
error:
   goto done ;
}

void trim( char *str )
{
   char *pbegin = str;
   char *pend = str + strlen(str) - 1 ; 
   while ( *pbegin )
   {
      if ( *pbegin == ' ' ||
           *pbegin == '\t' ||
           *pbegin == '\r' ||
           *pbegin == '\n')
      {
         ++pbegin;
      }
      else
      {
         break ;
      }
   }
   
   while ( *pend )
   {
      if ( *pend == ' ' ||
           *pend == '\t' ||
           *pend == '\r' ||
           *pend == '\n')
      {
         *pend = 0 ;
         --pend;
      }
      else
      {
         break ;
      }
   }
   
   memcpy( str, pbegin, pend - pbegin + 1 ) ;
   str[pend - pbegin + 1] = 0 ;
}

INT32 changeNodeConf( const CHAR* svc, const CHAR* conf, const CHAR* value )
{
   INT32 rc = SDB_OK ;

   CHAR installPath[ MAX_NAME_SIZE+1 ] = { 0 } ;
   CHAR confFile[ MAX_NAME_SIZE+1 ] = { 0 } ;
   CHAR bakConfFile[ MAX_NAME_SIZE+1 ] = { 0 } ;
   FILE* fpOld, *fpNew = NULL ;
   CHAR buffer[ MAX_NAME_SIZE+1 ] = { 0 } ;
   sprintf( buffer, "%s%s%s", conf, "=", value ) ;
   CHAR s[ MAX_NAME_SIZE ] ={0};
   INT32 len = 0 ;

   rc = getInstallPath( installPath ) ;
   CHECK_RC( rc, "fail to get installPath" ) ;
   sprintf( bakConfFile, "%s%s%s%s", installPath, "/conf/local/", svc, "/sdb.conf.bak" ) ;
   sprintf( confFile, "%s%s%s%s", installPath, "/conf/local/", svc, "/sdb.conf" ) ;
   rc = rename( confFile, bakConfFile ) ;
   CHECK_RC( rc, "fail to rename" ) ;
   
   fpOld = fopen( bakConfFile, "r" ) ;
   fpNew = fopen( confFile, "w+" ) ;
   if( !fpOld || !fpNew )
   {
      printf( "fail to open conf file: %s|%s\n", confFile, bakConfFile ) ;
      goto error ;
   }

   while( fgets( s, sizeof( s ), fpOld ) )
   {
      //len += strlen( s ) ;
      trim( s ) ;
      CHAR* idx = strstr( s, conf ) ;
      if( idx && idx == s )
      {
         //len -= strlen( s ) ;
         fprintf( fpNew, "%s\n", buffer ) ;
      }
      else
      {
         fputs( s, fpNew ) ; 
         fputc( '\n', fpNew ) ;
      }
      //rc = fputs( s, fpNew ) ; 
      //CHECK_RC( EOF, rc, "fail to fputs" ) ;
   }
   /*
   if( fseek( fp, len, SEEK_SET ) )
   {
      printf( "fail to seek file,file: %s, offset: %d\n", confFile, len ) ;
      goto error ;
   }*/
   //fprintf( fp, "%s", buffer ) ;
   //fclose( fp ) ;
   
done:
   if ( NULL != fpNew )
   {
      fclose( fpNew );
   }
   
   if ( NULL != fpOld )
   {
      fclose( fpOld );
      unlink( bakConfFile ) ;
   }
   return rc ;
error:
   rc = SDB_TEST_ERROR ;
   
   goto done ;
}


BOOLEAN checkConfig( const CHAR* svc, const char* key, const char* val )
{
   CHAR confFile[ MAX_NAME_SIZE+1 ] = { 0 } ;
   CHAR installPath[ MAX_NAME_SIZE+1 ] = { 0 } ;
   CHAR s[ MAX_NAME_SIZE ] ={0};
   INT32 rc = SDB_OK ;
   FILE* fp = NULL ;
   CHAR* pos = NULL ;
   
   rc = getInstallPath( installPath ) ;
   if ( rc != SDB_OK )
   {
      printf( "fail to get installPath\n") ;
      goto error ;
   }
 
   sprintf( confFile, "%s%s%s%s", installPath, "/conf/local/", svc, "/sdb.conf" ) ;
   
   fp = fopen( confFile, "r" ) ;
   if( !fp )
   {
      printf( "fail to open conf file: %s\n", confFile) ;
      goto error ;
   }

   while( fgets( s, sizeof( s ), fp ) )
   {
      CHAR* idx = NULL ;
      trim( s ) ;
      idx = strstr( s, key ) ;
      if( idx && idx == s )
      {
         break ;
      }
   }
   
   
   pos = strstr( s, "=" ) ;
   if ( NULL != pos && *(pos+1) != '\0' )
   {
      pos = pos + 1;
      trim( pos ) ;
      if ( 0 == strncmp( pos, val, strlen(pos) ) )
      {
         return TRUE ;
      }
   }
error:
   return FALSE ;
}

TEST( reloadConf, weight )
{
   INT32 rc = SDB_OK ;
   BOOLEAN res = FALSE ;

   sdbConnectionHandle db = SDB_INVALID_HANDLE ;
   sdbConnectionHandle dataDb = SDB_INVALID_HANDLE ;
   BOOLEAN isMaster = FALSE ;
   rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb" ;

   // create a slave node
   sdbReplicaGroupHandle rg = SDB_INVALID_HANDLE ;
   sdbNodeHandle node = SDB_INVALID_HANDLE ;
   const CHAR *host, *svc ;
   const CHAR *dataHost, *dataSvc ;
   rc = createSlaveNode( db, &rg, &node, &host, &svc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "node: host %s, svc %s\n", host, svc ) ;

   // start node
   rc = sdbStartNode( node ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to start node" ;
   
   /*
   // create cs cl and insert doc in rg in case rg have no dps log
   const CHAR* csName = "reloadConfTestCs" ;
   const CHAR* clName = "reloadConfTestCl" ;
   rc = createCsClInRg( db, rg, csName, clName ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = insertDoc( db, csName, clName ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // wait sync
   rc = waitSync( rg, host, svc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   */
   // change slave node weight to 20
   rc = changeNodeConf( svc, "weight", "20" ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   // reload conf
   rc = sdbReloadConfig( db, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to reload conf" ;
   
   rc = sdbGetNodeAddr( node, &dataHost, &dataSvc, NULL, NULL) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to exec sdbGetNodeAddr" ;
   rc = sdbConnect( dataHost, dataSvc, USER, PASSWD, &dataDb) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to exec sdbConnect(" << dataHost << "," << dataSvc <<")" ;
   rc = sdbFlushConfigure( dataDb, NULL );
   ASSERT_EQ( SDB_OK, rc ) << "fail to exec sdbFlushConfigure" ;
   
   res = checkConfig( svc, "weight", "20" ) ;
   ASSERT_EQ( res, TRUE ) << "fail to exec sdbReloadConfig" ;
   
   /*
   // reelect and check master
   bson option ;
   bson_init( &option ) ;
   bson_append_int( &option, "Seconds", 60 ) ;
   bson_finish( &option ) ;
   rc = sdbReelect( rg, &option ) ;
   bson_destroy( &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to reelect in rg" ;
   
   rc = isMasterNode( rg, host, svc, &isMaster ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   if( isMaster )
   {
      printf( "node %s:%s is master node.\n", host, svc ) ;
   }
   else
   {
      printf( "node %s:%s is not master node.\n", host, svc ) ;
   }
   ASSERT_TRUE( isMaster ) << "fail to check node to be master after reelect" ;	

   // drop cs 
   rc = sdbDropCollectionSpace( db, csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;  
   */
   // stop and remove node
   rc = sdbStopNode( node ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to stop node" ;
   do
   {
      ossSleep( 10 ) ;
      rc = isMasterNode( rg, host, svc, &isMaster ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to check node is master node or not" ;
   } while( isMaster ) ;
   rc = sdbRemoveNode( rg, host, svc, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to remove node" ;

   sdbDisconnect( db ) ;
   sdbReleaseConnection( db ) ;
   sdbDisconnect( dataDb ) ;
   sdbReleaseConnection( dataDb ) ;
   sdbReleaseReplicaGroup( rg ) ;
   sdbReleaseNode( node ) ;
}
