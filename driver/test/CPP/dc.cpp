#include <stdio.h>
#include <gtest/gtest.h>
#include "client.hpp"
#include "testcommon.hpp"
#include <string>
#include <iostream>

using namespace std ;
using namespace sdbclient ;


TEST( dc, not_connect )
{
   sdbDataCenter dc ;
   BSONObj obj ;
   INT32 rc = SDB_OK ;

   rc = dc.getDetail( obj ) ;
   ASSERT_EQ( SDB_NOT_CONNECTED, rc ) ;
}

TEST( dc, all_api_test )
{
   sdb db ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cursor ;
   sdbDataCenter dc ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   const CHAR *pStr                         = NULL ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;

   // connect to database
   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // getDC
   rc = db.getDC( dc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = dc.activateDC() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // getDetail
   rc = dc.getDetail( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "Before opration, dc detail is: " << obj.toString( FALSE, TRUE ) << endl ;

   // getName
   pStr = dc.getName() ;
   cout << "DC's name is: " << pStr << endl ;

   // disableReadonly
   rc = dc.enableReadOnly( FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // enableReadonly 
   rc = dc.enableReadOnly( TRUE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // activateDC
   rc = dc.activateDC() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // deactivateDC
   rc = dc.deactivateDC() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // disableReadonly
   rc = dc.enableReadOnly( FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // disconnect the connection
   db.disconnect() ;
}

