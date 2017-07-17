/**************************************************************
* @Description: test case for Jira questionaire Task
*				SEQUOIADBMAINSTREAM-2165
*				seqDB-10999:renameCS
*				seqDB-11000:renameCL
* @Modify     : Liang xuewang Init
*			 	2017-01-22
***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../common/testcommon.hpp"

sdbConnectionHandle db   = SDB_INVALID_HANDLE ;
sdbCSHandle cs           = SDB_INVALID_HANDLE ;
sdbCollectionHandle cl   = SDB_INVALID_HANDLE ;
const char* csModOldName = "renameCSTestCs1" ;
const char* csModNewName = "renameCSTestCs2" ;
char csOldName[100]      = ""  ;
char csNewName[100]      = "" ;
const char* clOldName    = "renameCLTestCl1" ;
const char* clNewName    = "renameCLTestCl2" ;

int checkCsExist( sdbConnectionHandle db, const char* csName, bool* exist )
{
	int rc = SDB_OK ;
	sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
	bson obj ;
    bson_init( &obj ) ;
	*exist = false ;

    rc = sdbListCollectionSpaces( db, &cursor ) ;
    CHECK_RC( rc, "fail to list cs, rc = %d\n", rc ) ;
   
    while( !( rc = sdbNext( cursor, &obj ) ) )
    {
        bson_iterator it ;
        bson_iterator_init( &it, &obj ) ;
        const char* name = bson_iterator_string( &it ) ;
        if( !strcmp( name, csName ) )
		{
            *exist = true ;
			break ;
		}
        bson_destroy( &obj ) ;
        bson_init( &obj ) ;
    }
	rc = SDB_OK ;

done:
    bson_destroy( &obj ) ;
    sdbReleaseCursor( cursor ) ;
	return rc ;
error:
	goto done ;
}

int checkClExist( sdbConnectionHandle db, const char* clFullName, bool* exist )
{
    int rc = SDB_OK ;
    sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
	bson obj ;
    bson_init( &obj ) ;
    *exist = false ;

    rc = sdbListCollections( db, &cursor ) ;
    CHECK_RC( rc, "fail to list cl, rc = %d\n", rc ) ;
    
    while( !( rc = sdbNext( cursor, &obj ) ) )
    {
        bson_iterator it ;
        bson_iterator_init( &it, &obj ) ;
        const char* name = bson_iterator_string( &it ) ;
        if( !strcmp( name, clFullName ) )
        {
            *exist = true ;
            break ;
        }
        bson_destroy( &obj ) ;
        bson_init( &obj ) ;
    }
	rc = SDB_OK ;

done:
    bson_destroy( &obj ) ;
    sdbReleaseCursor( cursor ) ;
    return rc ;
error:
	goto done ;
}

int checkBasicOperation( sdbCollectionHandle cl )
{
	int rc = SDB_OK ;
	bson record ;
    bson_init( &record ) ;
	bson selector ;
    bson_init( &selector ) ;
	bson obj ;
    bson_init( &obj ) ;
	sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
	bson_iterator it ;
	int result ;

	// insert
	bson_append_int( &record, "a", 1 ) ;
	bson_finish( &record ) ;
	rc = sdbInsert( cl, &record ) ;
	CHECK_RC( rc, "fail to insert record, rc = %d\n", rc ) ;

	// query
	bson_append_string( &selector, "a", "" ) ;
	bson_finish( &selector ) ;
	rc = sdbQuery( cl, &record, &selector, NULL, NULL, 0, -1, &cursor ) ;
	CHECK_RC( rc, "fail to query record, rc = %d\n", rc ) ;

	// check query result
	rc = sdbNext( cursor, &obj ) ;
	CHECK_RC( rc, "fail to get next in cursor, rc = %d\n", rc ) ;
	bson_iterator_init( &it, &obj ) ;
	result = bson_iterator_int( &it ) ;
	if( result != 1 )
	{
		printf( "fail to check query result,expect: %d,actual: %d\n", 1, result ) ;
		rc = SDB_DMS_RECORD_NOTEXIST ;
		goto error ;
	}

done:
	bson_destroy( &record ) ;
    bson_destroy( &selector ) ;
	bson_destroy( &obj ) ;
	sdbReleaseCursor( cursor ) ;
	return rc ; 
error:
	goto done ;
}

TEST( rename, renameCS )
{
	int rc = SDB_OK ;

	// connect to sdb
	getConf() ;
	rc = sdbConnect( HOSTNAME, SVCNAME, USER, PASSWD, &db ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to connect to sdb" ;
	if( !isStandalone( db ) )
	{
		sdbReleaseConnection( db ) ;
        return ;
	}

	// create cs with old name
	getUniqueName( csModOldName, csOldName ) ;
    rc = sdbCreateCollectionSpace( db, csOldName, SDB_PAGESIZE_4K, &cs ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to create cs with old name " << csOldName ;

	// rename cs with new name
	getUniqueName( csModNewName, csNewName ) ;
	bson option ;
	bson_init( &option ) ;
	bson_append_bool( &option, "Global", true ) ;
	bson_finish( &option ) ;
	rc = sdbRenameCollectionSpace( db, csOldName, csNewName, &option ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to rename cs oldName: " << csOldName << " newName: " << csNewName ; 
	
	// check rename cs
	bool exist ;
	rc = checkCsExist( db, csOldName, &exist ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to check cs exist after rename oldName: " << csOldName ;
	ASSERT_FALSE( exist ) << "fail to check rename cs " << csOldName ;
	rc = checkCsExist( db, csNewName, &exist ) ;
    ASSERT_EQ( rc, SDB_OK ) << "fail to check cs exist after rename newName: " << csNewName ;
    ASSERT_TRUE( exist ) << "fail to check rename cs " << csNewName ;

	// create cl with cs
	sdbReleaseCS( cs ) ;
	rc = sdbGetCollectionSpace( db, csNewName, &cs ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to get cs with newName: " << csNewName ;
	rc = sdbCreateCollection( cs, clOldName, &cl ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to create cl " << clOldName ;

	// rename cl with new name
	rc = sdbRenameCollection( cs, clOldName, clNewName, &option ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to rename cl olName: " << clOldName << " newName: " << clNewName ;
	
	// check rename cl
	char clOldFullName[200] ;
	sprintf( clOldFullName, "%s.%s", csNewName, clOldName ) ;
	char clNewFullName[200] ;
	sprintf( clNewFullName, "%s.%s", csNewName, clNewName ) ;
	rc = checkClExist( db, clOldFullName, &exist ) ;
    ASSERT_EQ( rc, SDB_OK ) << "fail to check cl exist after rename oldName: " << clOldFullName ;
    ASSERT_FALSE( exist ) << "fail to check rename cl " << clOldFullName ;
    rc = checkClExist( db, clNewFullName, &exist ) ;
    ASSERT_EQ( rc, SDB_OK ) << "fail to check cl exist after rename newName: " << clNewFullName ;
    ASSERT_TRUE( exist ) << "fail to check rename cl " << clNewFullName ;

	// check basic operation with new name cl
	sdbReleaseCollection( cl ) ;
	rc = sdbGetCollection1( cs, clNewName, &cl ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to get cl " << clNewName ;
	rc = checkBasicOperation( cl ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to check basic operation after rename cl" ;

	// drop cs and disconnect
	rc = sdbDropCollectionSpace( db, csNewName ) ;
	ASSERT_EQ( rc, SDB_OK ) << "fail to drop cs " << csNewName ;
	sdbDisconnect( db ) ;
	sdbReleaseCollection( cl ) ;
	sdbReleaseCS( cs ) ;
	sdbReleaseConnection( db ) ;	
}
