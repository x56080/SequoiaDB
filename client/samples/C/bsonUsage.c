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

   
*******************************************************************************/
#include <stdio.h>
#include "common.h"


// { "field_int": 100, "field_doule": 3.14, "field_str": "hello world", "field_bool": true, "field_null": null }
void genMiscFieldsRecord()
{
   INT32 rc = SDB_OK ;
   bson obj ;
   bson_init( &obj ) ;
   rc = bson_append_int( &obj, "field_int", 100 ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_double( &obj, "field_doule", 3.14 ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_string ( &obj, "field_str", "hello world" ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_bool( &obj, "field_bool", 1 ) ; // means true
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_null( &obj, "field_null" ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_finish( &obj ) ;
   CHECK_RC ( rc, "Error happen" ) ;

   printf( "the misc fields record is: \n" ) ;
   bson_print( &obj ) ;
   printf( "\n" ) ;

done:
   bson_destroy( &obj ) ;
   return ;
error:
   goto done ;
}

// { "field_oid": { "$oid": "56ab1bf749cd933667000000" } }
void genOIDRecord()
{
   INT32 rc = SDB_OK ;
   bson obj ;
   bson_oid_t oid;
   bson_oid_gen( &oid );
   bson_init( &obj ) ;
   rc = bson_append_oid( &obj, "field_oid", &oid );
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_finish( &obj ) ;
   CHECK_RC ( rc, "Error happen" ) ;

   printf( "the oid field record is: \n" ) ;
   bson_print( &obj ) ;
   printf( "\n" ) ;

done:
   bson_destroy( &obj ) ;
   return ;
error:
   goto done ;
}

//{ "field_date": { "$date": "2015-01-01" }, "field_timestamp": { "$timestamp": "2015-01-01-00.00.00.000000" } }
void genDateAndTimestampRecord()
{
   INT32 rc = SDB_OK ;
   bson obj ;
   bson_date_t datet = 1420041600000 ; // 2015-01-01
   bson_timestamp_t timestamp = { 0, 1420041600 } ; // 2015-01-01-00.00.00.000000

   bson_init( &obj ) ;
   rc = bson_append_date( &obj, "field_date", datet ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_timestamp( &obj, "field_timestamp", &timestamp ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_finish( &obj ) ;
   CHECK_RC ( rc, "Error happen" ) ;

   printf( "the date and timestamp fields record is: \n" ) ;
   bson_print( &obj ) ;
   printf( "\n" ) ;

done:
   bson_destroy( &obj ) ;
   return ;
error:
   goto done ;
}

// { "field_binary": { "$binary": "aGVsbG8gd29ybGQ=", "$type": "0" } } 
void genBinaryRecord()
{
   INT32 rc = SDB_OK ;
   bson obj ;
   // gen binary data
   CHAR *pStr = "aGVsbG8gd29ybGQ=" ; // the base64 code of "hello world"
   int len = getDeBase64Size ( pStr ) ;
   char *out = (char *)malloc( len ) ;
   memset( out, 0, len ) ;
   base64Decode( pStr, out, len ) ;
   printf( "\"%s\" is the base64 code of \"%s\"\n", pStr, out ) ;

   // build bson
   bson_init( &obj ) ;
   rc = bson_append_binary( &obj, "field_binary", 0, out, strlen(out) ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_finish( &obj ) ;
   CHECK_RC ( rc, "Error happen" ) ;

   printf( "the binary field record is: \n" ) ;
   // note: we put "hello world" into obj in binary format,
   // but bson_print display it as base64 code format
   bson_print( &obj ) ;
   printf( "\n" ) ;

done:
   bson_destroy( &obj ) ;
   free( out ) ;
   return ;
error:
   goto done ;
}

// { "field_regex": { "$regex": "^abc", "$options": "i" } }
void genRegexRecord()
{
   INT32 rc = SDB_OK ;
   const CHAR *pPatten = "^abc" ;
   const CHAR *pOptions = "i" ;
   bson obj ;
   bson_init( &obj ) ;
   rc = bson_append_regex( &obj, "field_regex", pPatten, pOptions ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_finish( &obj ) ;
   CHECK_RC ( rc, "Error happen" ) ;

   printf( "the regex field record is: \n" ) ;
   bson_print( &obj ) ;
   printf( "\n" ) ;

done:
   bson_destroy( &obj ) ;
   return ;
error:
   goto done ;
}

// { info:{ name:"Tom", age: 27, phone:["13800138000", 02066666666]}}
void genObjectRecord()
{
   INT32 rc = SDB_OK ;
   bson obj ;
   bson_init( &obj ) ;
   rc = bson_append_start_object( &obj, "info" ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_string( &obj, "name", "Tom" ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_int( &obj, "age", 27 ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_start_array( &obj, "phone" ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_string( &obj, "0", "13800138000" ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_string( &obj, "1", "02066666666" ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_finish_array( &obj ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_finish_object( &obj ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_finish( &obj ) ;
   CHECK_RC ( rc, "Error happen" ) ;

   printf( "the object field record is: \n" ) ;
   bson_print( &obj ) ;
   printf( "\n" ) ;

done:
   bson_destroy( &obj ) ;
   return ;
error:
   goto done ;
}

// { arr: [ 0, 1, 2 ] }
void genArrayRecord()
{
   INT32 rc = SDB_OK ;
   bson obj ;
   bson_init ( &obj ) ;
   rc = bson_append_start_array ( &obj, "arr" ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_int ( &obj, "0", 0 ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_int ( &obj, "1", 1 ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_int ( &obj, "2", 2 ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_append_finish_array ( &obj ) ;
   CHECK_RC ( rc, "Error happen" ) ;
   rc = bson_finish ( &obj ) ;
   CHECK_RC ( rc, "Error happen" ) ;

   printf( "the array field record is: \n" ) ;
   bson_print( &obj ) ;
   printf( "\n" ) ;

done:
   bson_destroy( &obj ) ;
   return ;
error:
   goto done ;
}

INT32 main ( INT32 argc, CHAR **argv )
{
   genMiscFieldsRecord() ;
   genOIDRecord() ;
   genArrayRecord() ;
   genObjectRecord() ;
   genDateAndTimestampRecord() ;
   genBinaryRecord() ;
   genRegexRecord() ;

   return 0;
}
