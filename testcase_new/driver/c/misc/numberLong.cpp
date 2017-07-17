/*****************************************************************************
*@Description : seqDB-7546:c_输入strict格式，查询显示
                seqDB-7547:c_strict格式的参数校验
                seqDB-7548:c_strict格式的边界值校验
*@Modify List : 2016-3-29  Ting YU  Init
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <gtest/gtest.h>
#include <string.h>
#include "jstobs.h"
#include "client.h"
#include "../common/testcommon.hpp"

const char* CsModName = "c_driver_test";
char CSNAME[100] ;
char CLNAME[] = "numberLong";

/*
void createCollection( sdbCollectionHandle *cl )
{
   sdbConnectionHandle db = SDB_OK ;
   sdbCSHandle cs = SDB_OK ;
   //sdbCollectionHandle cl = SDB_OK ;
   INT32 rc = SDB_OK ;
   const CHAR *csName = "c_driver_test" ;
   const CHAR *clName = "numberLong" ;

   // connect to sdb
   rc = sdbConnect( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   // create collection space
   bson csOptions ;
   bson_init( &csOptions ) ;
   bson_append_int( &csOptions, "PageSize", 65536 ) ;
   bson_append_finish_object( &csOptions ) ;
   rc = sdbCreateCollectionSpaceV2( db, csName, &csOptions, &cs ) ;
   if( SDB_DMS_CS_EXIST == rc )
   {
      rc = sdbGetCollectionSpace( db, csName, &cs ) ;
   }
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &csOptions ) ;
   
   // create collection
   bson clOptions ;
   bson_init( &clOptions ) ;
   bson_append_int( &clOptions, "ReplSize", 0 ) ;
   bson_append_bool( &clOptions, "Compressed", true ) ;
   bson_append_finish_object( &clOptions ) ;
   rc = sdbDropCollection( cs, clName ) ;
   if( SDB_DMS_NOTEXIST == rc )
   {
      printf( "collection:%s don't exist\n", clName ) ;
   }
   else
   {
      printf( "success to drop collections\n" ) ;
   }
   rc = sdbCreateCollection1( cs, clName, &clOptions, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &clOptions ) ;
}
*/
void checkLongVal( const sdbCollectionHandle &cl, const long long longVal )
{
   INT32 rc = SDB_OK ;
   bson obj ;
   bson_iterator it;
   
   //number to string
   char longStr[25];
   sprintf( longStr, "%lld", longVal );

   //json to bson 
   char recJson[100];
   sprintf( recJson, "%s%s%s", "{ \"_id\": { \"$numberLong\": \"", longStr, "\" } }");
   bson_init( &obj ) ;
   jsonToBson( &obj, recJson );
   ASSERT_EQ( SDB_OK, rc ) ;  
   rc = bson_finish( &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
    
   bson_find( &it, &obj, "_id" ); 
   ASSERT_TRUE( bson_iterator_long(&it) == longVal );
                       
   //insert 
   rc = sdbDelete ( cl, NULL, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) ;     
   rc = sdbInsert ( cl, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   //query
   sdbCursorHandle cursor = 0 ;
   rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;
   
   INT32 cnt = 0;
   bson_init( &obj );
   while( !( sdbNext( cursor, &obj ) ) )
   {   
      //check value            
      bson_find( &it, &obj, "_id");      
      ASSERT_TRUE( bson_iterator_long(&it) == longVal );    

      bson_destroy( &obj );
      bson_init( &obj );
      cnt++;
   }
   ASSERT_TRUE( cnt == 1 );
   
   bson_destroy( &obj );
}

TEST( numberLong, boundary )
{         
   //create cl
   INT32 rc = SDB_OK;
   getUniqueName( CsModName,CSNAME ) ;
   sdbConnectionHandle db = 0 ;
   sdbCSHandle cs = 0 ;
   sdbCollectionHandle cl = 0 ;
   rc = createNormalCl( &db, &cs, &cl, CSNAME, CLNAME ); 
   ASSERT_EQ( rc, SDB_OK ) ;
   
   long long longMax = 9223372036854775807;
   long long longMin = -9223372036854775808;
   checkLongVal( cl, longMax ); 
   checkLongVal( cl, longMin ); 

   rc = sdbDropCollectionSpace( db, CSNAME ) ;
   ASSERT_EQ( rc, SDB_OK ) ;
   sdbDisconnect( db ) ;
   sdbReleaseCollection( cl ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseConnection( db ) ;
}

TEST( numberLong, outofBoundary )
{                    
   BOOLEAN rc ;
   bson obj ;

   //1. error fomart: {"$numberLong":"123.6"}       
   char recJson1[] = "{ \"a\": { \"$numberLong\": 9223372036854775808 } }" ;     
   bson_init( &obj ) ;
   rc = jsonToBson( &obj, recJson1 ); 
   ASSERT_EQ( FALSE, rc ) ;
    
   //2. error fomart: {"$numberLong":123}       
   char recJson2[] = "{ \"a\": { \"$numberLong\": -9223372036854775809 } }" ;
   bson_init( &obj ) ;
   rc = jsonToBson( &obj, recJson2 ); 
   ASSERT_EQ( FALSE, rc ) ;      
}

TEST( numberLong, errorFormat )
{            
   BOOLEAN rc ;
   bson obj ;
   
   //1. error fomart: {"$numberLong":"123.6"}       
   char recJson1[] = "{ \"a\": { \"$numberLong\": \"123.5\" } }" ;     
   bson_init( &obj ) ;
   rc = jsonToBson( &obj, recJson1 ); 
   ASSERT_EQ( FALSE, rc ) ;
    
   //2. error fomart: {"$numberLong":123}       
   char recJson2[] = "{ \"a\": { \"$numberLong\": 123.5 } }" ;
   bson_init( &obj ) ;
   rc = jsonToBson( &obj, recJson2 ); 
   ASSERT_EQ( FALSE, rc ) ;  
}
