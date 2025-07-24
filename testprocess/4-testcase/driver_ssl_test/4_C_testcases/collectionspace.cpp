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

   Source File Name = collectionspace.cpp

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
#include <gtest/gtest.h>
#include "testcommon.h"
#include "client.h"

TEST(collectonspace,sdbCreateCollection)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle collectionspace    = 0 ;
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbSecureConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbDropCollection ( collectionspace,
                            COLLECTION_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbCreateCollection ( collectionspace, COLLECTION_NAME,
                              &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collectonspace,sdbCreateCollection1_without_options)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle collectionspace    = 0 ;
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   bson option ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbSecureConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // drop the exist cl first
   rc = sdbDropCollection ( collectionspace, COLLECTION_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // build option
   bson_init( &option );
   bson_append_int( &option, "ReplSize", 0 );
   bson_finish( &option ) ;
   // create cl
   rc = sdbCreateCollection1 ( collectionspace,
                               COLLECTION_NAME,
                               &option,
                               &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_destroy( &option ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collectonspace,sdbCreateCollection1_with_options)
{
   printf("The test work in cluster environment\
 with at least 2 data notes.\n") ;
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle collectionspace    = 0 ;
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   bson options ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbSecureConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // drop the exist cl first
   rc = sdbDropCollection ( collectionspace, COLLECTION_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // option is {ShardingKey:{age:1,name:-1}},{ReplSize:2},{Compressed:true}
   bson_init ( &options ) ;
   bson_append_start_object ( &options, "ShardingKey" ) ;
   bson_append_int ( &options, "age", 1 ) ;
   bson_append_int ( &options, "name", -1 ) ;
   bson_append_finish_object ( &options ) ;
   bson_append_int ( &options, "ReplSize", 2 ) ;
   bson_append_bool ( &options, "Compressed", true ) ;
   bson_finish ( &options ) ;
   bson_print ( &options ) ;
   // get cl with sharding info
   rc = sdbCreateCollection1 ( collectionspace,
                               COLLECTION_NAME,
                               &options,
                               &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy ( &options ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collectonspace,sdbDropCollection)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle collectionspace    = 0 ;
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbSecureConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // create cl
   rc = sdbCreateCollection ( collectionspace, COLLECTION_NAME,
                              &collection ) ;
   // drop cl
   rc = sdbDropCollection ( collectionspace, COLLECTION_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collectonspace,sdbGetCollection1)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle collectionspace    = 0 ;
   sdbCollectionHandle collection = 0 ;
   INT32 rc                       = SDB_OK ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbSecureConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl handle
   rc = sdbGetCollection1 ( collectionspace, COLLECTION_NAME, &collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   sdbDisconnect ( connection ) ;
   sdbReleaseCollection ( collection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
}

TEST(collectonspace,sdbGetCSName)
{
   sdbConnectionHandle connection = 0 ;
   sdbCSHandle collectionspace    = 0 ;
   INT32 rc                       = SDB_OK ;
   CHAR *pCSName                  = NULL ;
   // memset ( pCSName, 0, sizeof(char)*128 ) ;
   rc = initEnv( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = sdbSecureConnect ( HOST, SERVER, USER, PASSWD, &connection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( connection,
                             COLLECTION_SPACE_NAME,
                             &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs name
   rc = sdbGetCSName ( collectionspace, &pCSName ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf("CS name is :%s\n",pCSName ) ;
   sdbDisconnect ( connection ) ;
   sdbReleaseCS ( collectionspace ) ;
   sdbReleaseConnection ( connection ) ;
}

