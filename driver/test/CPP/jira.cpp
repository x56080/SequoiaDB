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

   Source File Name = jira.cpp

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
#include "client.hpp"
#include "testcommon.hpp"
#include <string>
#include <iostream>

using namespace std ;
using namespace sdbclient ;

TEST( jira, bug_8345 )
{
   sdb db ;
   BSONObj result ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;

   // connect to database
   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   db.dropCollectionSpace( COLLECTION_SPACE_NAME ) ;
   // case 1: test get last error
   {
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCollection cl2 ;
   rc = db.createCollectionSpace( COLLECTION_SPACE_NAME, SDB_PAGESIZE_4K, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
   rc = cs.createCollection( "test", cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
   rc = cs.createCollection( "test", cl2 ) ;
   rc = db.getLastErrorObj( result ) ;
   ASSERT_EQ( SDB_OK, rc ) << "failed to get last error object" ;
   printf("error obj is: %s\n", result.toString(false, true, false).c_str() ) ;
   }
   rc = db.getLastErrorObj( result ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "failed to get last error object" ;

   // case 2: test get last result
   {
   sdbCollection cl ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.getCollection( COLLECTION_FULL_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.insert( BSON( "a" << 1 ) ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.getLastResultObj( result ) ;
   ASSERT_EQ( SDB_OK, rc ) << "failed to get last error object" ;
   printf("result obj is: %s\n", result.toString(false, true, false).c_str() ) ; 
   }
   rc = db.getLastResultObj( result ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "failed to get last result object" ;

   rc = db.dropCollectionSpace( COLLECTION_SPACE_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
   db.disconnect() ;
}
