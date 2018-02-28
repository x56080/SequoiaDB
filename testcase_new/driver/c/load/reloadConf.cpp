/**************************************************************
* @Description: test case for Jira questionaire Task
*					 SEQUOIADBMAINSTREAM-2165
*					 seqDB-11001:reloadConf
*               修改数据组备节点的配置文件weight=20
*					 重新选主，检查备节点升级为主节点
* @Modify     : Liang xuewang Init
*			 		 2017-01-22
***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <unistd.h>
#include "testcommon.hpp"

int createCsClInRg( sdbConnectionHandle db, sdbReplicaGroupHandle rg, 
                    const char* csName, const char* clName )
{
   int rc = SDB_OK ;
   char* rgName ;
   sdbCSHandle cs = SDB_INVALID_HANDLE ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   bson option ;
   bson_init( &option ) ;

   rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
   CHECK_RC( rc, "fail to create cs" ) ;
   rc = sdbGetReplicaGroupName( rg, &rgName ) ;
   CHECK_RC( rc, "fail to get rgName" ) ;
   bson_append_string( &option, "Group", rgName ) ;
   bson_append_int( &option, "ReplSize", 0 ) ;
   bson_finish( &option ) ;
   rc = sdbCreateCollection1( cs, clName, &option, &cl ) ;
   CHECK_RC( rc, "fail to create cl" ) ;
done:
   bson_destroy( &option ) ;
   sdbReleaseCollection( cl ) ;
   sdbReleaseCS( cs ) ;
   return rc ;
error:
   goto done ;
}

int getInstallPath( char* path )
{
   int rc = SDB_OK ;
   const char* installFile = "/etc/default/sequoiadb" ;
   char s[ MAX_NAME_SIZE+1 ] = { 0 } ;
   const char* installStr = "INSTALL_DIR=" ;
   int installStrLen = strlen( installStr ) ;
   int len ;

   FILE* fp = fopen( installFile, "r" ) ;
   if( !fp )
   {
      printf( "fail to open file /etc/default/sequoiadb\n" ) ;
      goto error ;
   }
   while( fgets( s, sizeof( s ), fp ) )
   {
      char* idx = strstr( s, installStr ) ;
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

int isMasterNode( sdbReplicaGroupHandle& rg, const char* host, const char* svc, bool* res )
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
int createSlaveNode( sdbConnectionHandle db, sdbReplicaGroupHandle* rg, sdbNodeHandle* node, 
					      const char** host, const char** svc )
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

int getLSN( sdbConnectionHandle db, int64_t* offset, int* version )
{
	int rc = SDB_OK ;
	sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
	bson sel; 
	bson_init( &sel ); 
	bson obj ;
   bson_init( &obj ) ;
	bson_iterator it, sub_it ;  

	bson_append_string( &sel, "CurrentLSN", "" ) ;
	bson_finish( &sel ) ; 
	rc = sdbGetSnapshot( db, SDB_SNAP_DATABASE, NULL, &sel, NULL, &cursor ) ;
	CHECK_RC( rc, "fail to get snapshot database\n" ) ;
	
	rc = sdbNext( cursor, &obj ) ;
	CHECK_RC( rc, "fail to get next\n" ) ;

	bson_find( &it, &obj, "CurrentLSN" ) ;
	bson_iterator_subiterator( &it, &sub_it ) ;
	bson_iterator_next( &sub_it ) ;
	*offset = bson_iterator_long( &sub_it ) ;
	bson_iterator_next( &sub_it ) ;
	*version = bson_iterator_int( &sub_it ) ;

   rc = sdbCloseCursor( cursor ) ;
   CHECK_RC( rc, "fail to close cursor\n" ) ; 

done:
	bson_destroy( &sel ) ;
	bson_destroy( &obj ) ;
	sdbReleaseCursor( cursor ) ;
	return rc ;
error:
	goto done ;
}

// wait sync finish, lsn is equal
int waitSync( sdbReplicaGroupHandle& rg, const char* host, const char* svc )
{
	int rc = SDB_OK ;
	sdbConnectionHandle db = SDB_INVALID_HANDLE ; 
   sdbConnectionHandle db1 = SDB_INVALID_HANDLE ;
	sdbNodeHandle master = SDB_INVALID_HANDLE ;
	const char *host1, *svc1 ;
	int64_t offset, offset1 ;
   int version, version1 ;

   rc = sdbGetNodeMaster( rg, &master ) ;
   CHECK_RC( rc, "fail to get master node\n", rc ) ;
   rc = sdbGetNodeAddr( master, &host1, &svc1, NULL, NULL ) ;
   CHECK_RC( rc, "fail to get master node addr\n", rc ) ;

	rc = sdbConnect( host, svc, USER, PASSWD, &db ) ;
	CHECK_RC( rc, "fail to connect node %s:%s\n", host, svc, rc ) ;
	rc = sdbConnect( host1, svc1, USER, PASSWD, &db1 ) ;
	CHECK_RC( rc, "fail to connect master node %s:%s\n", host1, svc1, rc ) ;
	
	do {
		rc = getLSN( db, &offset, &version ) ;
		CHECK_RC( rc, "fail to get lsn of node\n", rc ) ;
		rc = getLSN( db1, &offset1, &version1 ) ;
		CHECK_RC( rc, "fail to get lsn of master node\n", rc ) ;
	} while( offset != offset1 || version != version1 ) ;
	printf( "node offset: %ld,version: %d\n", offset, version ) ;
	printf( "master node offset: %ld,version: %d\n", offset1, version1 ) ; 

done:
	sdbDisconnect( db ) ;
	sdbDisconnect( db1 ) ;
	sdbReleaseConnection( db ) ;
	sdbReleaseConnection( db1 ) ;
	return rc ;
error:
	goto done ;
}

int changeNodeConf( const char* svc, const char* conf, const char* value )
{
	int rc = SDB_OK ;

   FILE* fp = NULL ;
	char installPath[ MAX_NAME_SIZE+1 ] = { 0 } ;
   char confFile[ MAX_NAME_SIZE+1 ] = { 0 } ;   
   char buffer[ MAX_NAME_SIZE+1 ] = { 0 } ;
   char s[ MAX_NAME_SIZE+1 ] = { 0 } ;
   int len = 0 ;

	getInstallPath( installPath ) ;	
	sprintf( confFile, "%s%s%s%s", installPath, "/conf/local/", svc, "/sdb.conf" ) ;
	fp = fopen( confFile, "r+" ) ;
	if( !fp )
	{
		printf( "fail to open conf file: %s\n", confFile ) ;
		goto error ;
	}
	sprintf( buffer, "%s%s%s", conf, "=", value ) ;
	while( fgets( s, sizeof( s ), fp ) )
	{
		len += strlen( s ) ;
		char* idx = strstr( s, conf ) ;
		if( idx )
		{
			len -= strlen( s ) ;
        	break ;
		}
	}
	if( fseek( fp, len, SEEK_SET ) )
	{
		printf( "fail to seek file,file: %s,offset: %d\n", confFile, len ) ;
		goto error ;
	}
	fprintf( fp, "%s", buffer ) ;
	fclose( fp ) ;

done:
	return rc ;
error:
   rc = SDB_TEST_ERROR ;
   goto done ;
}

TEST( reloadConf, weight )
{
	int rc = SDB_OK ;
	sdbConnectionHandle db = SDB_INVALID_HANDLE ;

	getConf() ;
	rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb" ;
	if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }

	// create and start a slave node
	sdbReplicaGroupHandle rg = SDB_INVALID_HANDLE ;
	sdbNodeHandle node = SDB_INVALID_HANDLE ;
	const char *host, *svc ;
	rc = createSlaveNode( db, &rg, &node, &host, &svc ) ;
	ASSERT_EQ( SDB_OK, rc ) ;
	printf( "node: name %s, svc %s\n", host, svc ) ;
	rc = sdbStartNode( node ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to start node" ;

   // create cs cl in rg in case rg have no dps log
   const CHAR* csName = "reloadConfTestCs" ;
   const CHAR* clName = "reloadConfTestCl" ;
   rc = createCsClInRg( db, rg, csName, clName ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

	rc = waitSync( rg, host, svc ) ;
	ASSERT_EQ( SDB_OK, rc ) ;
	
	// change slave node weight to 20
	rc = changeNodeConf( svc, "weight", "20" ) ;
	ASSERT_EQ( SDB_OK, rc ) ;

 	// reload conf
   rc = sdbReloadConfig( db, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to reload conf" ;

	// reelect and check master
	bson option ;
	bson_init( &option ) ;
	bson_append_int( &option, "Seconds", 60 ) ;
	bson_finish( &option ) ;
	rc = sdbReelect( rg, &option ) ;
	bson_destroy( &option ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to reelect in rg" ;
	bool isMaster = false ;
	rc = isMasterNode( rg, host, svc, &isMaster ) ;
	ASSERT_EQ( rc, SDB_OK ) ;
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

	// stop and remove node
	rc = sdbStopNode( node ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to stop node after test" ;
	do {
      sleep( 1 ) ;
		rc = isMasterNode( rg, host, svc, &isMaster ) ;
		ASSERT_EQ( rc, SDB_OK ) << "fail to check node is master node or not" ;
	} while( isMaster ) ;
	rc = sdbRemoveNode( rg, host, svc, NULL ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to remove node after test" ;

	// disconnect and release
	sdbDisconnect( db ) ;
	sdbReleaseConnection( db ) ;
	sdbReleaseReplicaGroup( rg ) ;
	sdbReleaseNode( node ) ;
}
