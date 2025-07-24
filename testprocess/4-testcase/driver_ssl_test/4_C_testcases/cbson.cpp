/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = cbson.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <gtest/gtest.h>
#include "testcommon.h"
#include "client.h"
#include "base64c.h"

#define USERDEF       "sequoiadb"
#define PASSWDDEF     "sequoiadb"

TEST(cbson, empty)
{
   INT32 rc = SDB_OK ;
   ASSERT_EQ( rc, SDB_OK ) ;
}

TEST(cbson, binary)
{
   INT32 rc = SDB_OK ;
   bson obj ;
   const char *str = "{ \"key\": { \"$binary\" : \"aGVsbG8gd29ybGQ=\", \"$type\": \"1\" } }" ;
   BOOLEAN flag = jsonToBson2( &obj, str, FALSE, FALSE ) ;
   ASSERT_EQ( flag, TRUE ) ;
   bson_print( &obj ) ;
   ASSERT_EQ( rc, SDB_OK ) ;
}


TEST(cbson, esc_problem)
{
   sdbConnectionHandle connection  = 0 ;
   sdbConnectionHandle connection1 = 0 ;
   sdbCSHandle cs                 = 0 ;
   sdbCollectionHandle cl         = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   bson obj1 ;
   bson obj2 ;
   bson obj3 ;
   bson obj4 ;
   bson obj5 ;
   bson obj6 ;
   bson obj7 ;
   bson obj8 ;
   bson obj9 ;
   bson temp ;
   bson record ;
   BOOLEAN flag = FALSE ;
   const CHAR *str1 = "{\"1\":\"1\\\"1\"}";
   const CHAR *str2 = "{\"2\":\"2\\\\2\"}";
   const CHAR *str3 = "{\"3\":\"3\\/3\"}";
   const CHAR *str4 = "{\"4\":\"123\\b4\"}";
   const CHAR *str5 = "{\"5\":\"5\\f5\"}";
   const CHAR *str6 = "{\"6\":\"6\\n6\"}";
   const CHAR *str7 = "{\"7\":\"7\\r7\"}";
   const CHAR *str8 = "{\"8\":\"8\\t8\"}";
   const CHAR *str9 = "{\"a\":\"a\\u0041a\"}";
   const int num = 10;
   const int bufSize = 100;
   CHAR* result[num];
   int i = 0;
   for ( ; i < num; i++ )
   {
      result[i] = (CHAR *)malloc(bufSize);
      if ( result[i] == NULL)
         printf( "Failed to malloc.\n" );
   }
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbSecureConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection ( connection, COLLECTION_FULL_NAME, &cl ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // TO DO:
   bson_init( &obj1 ) ;
   bson_init( &obj2 ) ;
   bson_init( &obj3 ) ;
   bson_init( &obj4 ) ;
   bson_init( &obj5 ) ;
   bson_init( &obj6 ) ;
   bson_init( &obj7 ) ;
   bson_init( &obj8 ) ;
   bson_init( &obj9 ) ;
   flag = jsonToBson( &obj1, str1 ) ;
   ASSERT_EQ( TRUE, flag ) ;
   flag = jsonToBson( &obj2, str2 ) ;
   ASSERT_EQ( TRUE, flag ) ;
   flag = jsonToBson( &obj3, str3 ) ;
   ASSERT_EQ( TRUE, flag ) ;
   flag = jsonToBson( &obj4, str4 ) ;
   ASSERT_EQ( TRUE, flag ) ;
   flag = jsonToBson( &obj5, str5 ) ;
   ASSERT_EQ( TRUE, flag ) ;
   flag = jsonToBson( &obj6, str6 ) ;
   ASSERT_EQ( TRUE, flag ) ;
   flag = jsonToBson( &obj7, str7 ) ;
   ASSERT_EQ( TRUE, flag ) ;
   flag = jsonToBson( &obj8, str8 ) ;
   ASSERT_EQ( TRUE, flag ) ;
   flag = jsonToBson( &obj9, str9 ) ;
   ASSERT_EQ( TRUE, flag ) ;
   // insert
   rc = sdbInsert( cl, &obj1 );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ ( SDB_OK, rc ) ;
   rc = sdbInsert( cl, &obj2 );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ ( SDB_OK, rc ) ;
   rc = sdbInsert( cl, &obj3 );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ ( SDB_OK, rc ) ;
   rc = sdbInsert( cl, &obj4 );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ ( SDB_OK, rc ) ;
   rc = sdbInsert( cl, &obj5 );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ ( SDB_OK, rc ) ;
   rc = sdbInsert( cl, &obj6 );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ ( SDB_OK, rc ) ;
   rc = sdbInsert( cl, &obj7 );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ ( SDB_OK, rc ) ;
   rc = sdbInsert( cl, &obj8 );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ ( SDB_OK, rc ) ;
   rc = sdbInsert( cl, &obj9 );
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ ( SDB_OK, rc ) ;
   // query
   rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ ( SDB_OK, rc ) ;
   bson_init ( &temp );
   i = 0;
   while ( !( rc = sdbNext( cursor, &temp ) ) )
   {
      rc = bsonToJson( result[i], bufSize, &temp, FALSE, FALSE );
//      bson_print( &temp );
      ASSERT_EQ ( TRUE, rc );
      printf( "bson is: %s\n", result[i] );
      i++;
      bson_destroy( &temp );
      bson_init( &temp );
   }
   bson_destroy( &temp ) ;
   bson_destroy( &obj1 ) ;
   bson_destroy( &obj2 ) ;
   bson_destroy( &obj3 ) ;
   bson_destroy( &obj4 ) ;
   bson_destroy( &obj5 ) ;
   bson_destroy( &obj6 ) ;
   bson_destroy( &obj7 ) ;
   bson_destroy( &obj8 ) ;
   bson_destroy( &obj9 ) ;

   for ( i = 0; i < num; i++ )
   {
      free( result[i] );
   }

   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection( cl ) ;
   sdbReleaseCS( cs ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseConnection ( connection ) ;

}


/*
TEST(cbson, regex)
{
   sdbConnectionHandle db = 0 ;
   sdbCSHandle cs    = 0 ;
   sdbCollectionHandle cl = 0 ;
   sdbCursorHandle cursor         = 0 ;
   INT32 rc                       = SDB_OK ;
   bson obj ;
   bson rule ;
   bson cond ;
   CHAR buf[64] = { 0 } ;
   const CHAR* regex = "^31" ;
   const CHAR* options = "i" ;
   INT32 num = 10 ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbSecureConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( db,
                             COLLECTION_SPACE_NAME,
                             &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection ( db,
                        COLLECTION_FULL_NAME,
                        &cl ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // insert some recored
   for ( INT32 i = 0 ; i < num; i++ )
   {
      CHAR buff[32] = { 0 } ;
      CHAR bu[2] = { 0 } ;
      sprintf( bu,"%d",i ) ;
      strcat( buff, "31" ) ;
      strncat( buff, bu, 1 ) ;
      bson_init ( &obj ) ;
      bson_append_string( &obj, "name", buff  ) ;
      bson_append_int ( &obj, "age", 30 + i ) ;
      bson_finish( &obj ) ;
      rc = sdbInsert( cl, &obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      bson_destroy( &obj ) ;
   }

   // insert some recored
   for ( INT32 i = 0 ; i < num; i++ )
   {
      CHAR buff[32] = { 0 } ;
      CHAR bu[2] = { 0 } ;
      sprintf( bu, "%d", i ) ;
      strcat( buff, "41" ) ;
      strncat( buff, bu, 1 ) ;
      bson_init ( &obj ) ;
      bson_append_string( &obj, "name", buff  ) ;
      bson_append_int ( &obj, "age", 40 + i ) ;
      bson_finish( &obj ) ;
      rc = sdbInsert( cl, &obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      bson_destroy( &obj ) ;
   }

   // cond
   bson_init ( &cond ) ;

//   bson_append_regex( &cond, "name", regex, options ) ;

////
//   bson_append_start_object( &cond, "name" ) ;
//   bson_append_string ( &cond, "$regex", "^31" ) ;
//   bson_append_string ( &cond, "$options", "i" ) ;
//   bson_append_finish_object( &cond ) ;
////
   rc = bson_finish( &cond ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // rule
   bson_init ( &rule ) ;
   bson_append_start_object( &rule, "$set" ) ;
   bson_append_int ( &rule, "age", 999 ) ;
   bson_append_finish_object( &rule ) ;
   rc = bson_finish ( &rule ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // update with regex expression
   rc = sdbUpdate( cl, &rule, &cond, NULL ) ;
   CHECK_MSG("%s%d\n","rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // query
   rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // print the records
   displayRecord( &cursor ) ;

   sdbDisconnect ( db ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseCollection ( cl ) ;
   sdbReleaseCS ( cs ) ;
   sdbReleaseConnection ( db ) ;
}
*/

TEST( cbson, base64Encode )
{
   INT32 rc = SDB_OK ;
   const CHAR *str = "hello world" ;
   const CHAR *str2 = "aGVsbG8gd29ybGQ=" ;
   INT32 strLen = strlen( str) ;
   // encode
   INT32 len = getEnBase64Size( strLen ) ;
   CHAR *out = (CHAR *)malloc( len ) ;
   memset( out, 0, len ) ;
   base64Encode( str, strLen, out, len ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   std::cout << "out is: " << out << std::endl ;
   ASSERT_EQ( 0, strcmp( str2, out) ) ;
   free ( out ) ;
}

TEST( cbson, base64Decode )
{
   INT32 rc = SDB_OK ;
   const CHAR *str = "aGVsbG8gd29ybGQ=" ;
   const CHAR *str2 = "hello world" ;
   INT32 len = getDeBase64Size( str ) ;
   CHAR *out = (CHAR *)malloc( len ) ;
   memset( out, 0, len ) ;
   base64Decode( str, out, len ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   std::cout << "out is: " << out << std::endl ;
   ASSERT_EQ( 0, strcmp(str2, out ) ) ;
   free ( out ) ;
}
