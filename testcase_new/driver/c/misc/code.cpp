/**************************************************************
* @Description: test case for Jira questionaire
*				SEQUOIADBMAINSTREAM-2131
* @Modify:      Liang xuewang Init
*			 	2017-01-07
***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "../common/testcommon.hpp"

sdbConnectionHandle db = SDB_INVALID_HANDLE ;

TEST( codeTest, codeBSON )
{
	INT32 rc = SDB_OK ;

	// connect sdb
	getConf() ;
	rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to connect sdb" ;
	if( isStandalone( db ) )
	{
    	printf( "Run mode is standalone,cannot create procedure.\n" ) ;
      	sdbDisconnect( db ) ;
      	sdbReleaseConnection( db ) ;
      	return ;
   	}

	// create procedure
	const char *function = "function add(a,b) { return a+b ; }" ;
	rc = sdbCrtJSProcedure( db, function ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to create procedure" ;

	// list procedure
	bson condition ;
	bson_init( &condition ) ;
	bson_append_string( &condition, "name", "add" ) ;
	bson_finish( &condition ) ;
	sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
	rc = sdbListProcedures( db, &condition, &cursor ) ;
	bson_destroy( &condition ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to list procedures" ;

	// check procedure
	bson procedure ;
	bson_init( &procedure ) ;
	rc = sdbNext( cursor, &procedure ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to get next in cursor" ;
	bson_iterator it ;
	bson_type type = bson_find( &it, &procedure, "func" ) ;
	ASSERT_EQ( type, BSON_CODE ) << "fail to check bson type" ;
	const char *code = bson_iterator_code( &it ) ;
	ASSERT_STREQ( function, code ) ;  
	bson_destroy( &procedure ) ;
	sdbReleaseCursor( cursor ) ;

	// remove procedure
	rc = sdbRmProcedure( db, "add" ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to remove procedure" ;

	// disconnect
	sdbDisconnect( db ) ;
	// release handle
	sdbReleaseConnection( db ) ;
}
