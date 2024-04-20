#include <stdio.h>
#include <gtest/gtest.h>
#include "testcommon.h"
#include "client.h"

TEST(dc, dc_all_api)
{
   INT32 rc = SDB_OK ;
   // initialize local variables
   sdbConnectionHandle db     = 0 ;
   sdbCursorHandle cursor     = 0 ;
   sdbDCHandle dc             = 0 ;
#define DC_NAME 255
   CHAR dcName[ DC_NAME + 1 ] =  { 0 } ;
   bson obj ;
   bson_init( &obj ) ;
   // connect to database
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // getDC
   rc = sdbGetDC( db, &dc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // get name
   rc = sdbGetDCName( dc, dcName, DC_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // getDetail
   rc = sdbGetDCDetail( dc, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "Detail is: " ) ;
   bson_print(&obj) ;
   bson_destroy( &obj ) ;

   // disableReadonly
   rc = sdbEnableReadOnly( dc, FALSE ) ; 
   ASSERT_EQ( SDB_OK, rc ) ;

   // enableReadonly
   rc = sdbEnableReadOnly( dc, TRUE ) ; 
   ASSERT_EQ( SDB_OK, rc ) ;

   // activateDC
   rc = sdbActivateDC( dc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // deactivateDC
   rc = sdbDeactivateDC( dc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // disableReadonly
   rc = sdbEnableReadOnly( dc, FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // disconnect the connection
   sdbDisconnect ( db ) ;
   //release the local variables
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseDC ( dc ) ;
   sdbReleaseConnection ( db ) ;
}

