/**************************************************************************
 * @Description:   seqDB-23278:getCS()，getCL()指定checkExist参数
 * @Modify:        liuli Init
 *                 2021-02-07
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class getCSTest23278 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   const CHAR* csName1 ;
   const CHAR* clName1 ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   
   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "cs_23278" ;
	  csName1 = "cs_23278_1";
      clName = "cl_23278" ;
	  clName1 = "cl_23278_1" ;
      rc = db.createCollectionSpace( csName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
      rc = cs.createCollection( clName, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;   
   }

   void TearDown()
   {
      if( shouldClear() )
      {
         INT32 rc = db.dropCollectionSpace( csName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      }
      testBase::TearDown() ;
   }
} ;

TEST_F( getCSTest23278, getCS23278 )
{
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }

   INT32 rc = SDB_OK ;

   sdbCollectionSpace cs1 ;
   rc = db.getCollectionSpace( csName1,cs1,false );
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cs" ;

   sdbCollectionSpace cs2 ;
   rc = db.getCollectionSpace( csName,cs2,true );
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cs" ;

   sdbCollectionSpace cs3 ;
   rc = db.getCollectionSpace( csName1,cs3,true );
   ASSERT_EQ( SDB_DMS_CS_NOTEXIST, rc ) << "fail to get cs" ;

   sdbCollectionSpace cs4 ;
   rc = db.getCollectionSpace( csName1,cs4 );
   ASSERT_EQ( SDB_DMS_CS_NOTEXIST, rc ) << "fail to get cs" ;

   sdbCollectionSpace cs5 ;
   rc = db.getCollectionSpace( csName,cs5 );
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cs" ;

   sdbCollection cl1 ;
   rc = cs5.getCollection( clName1,cl1,false );
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cl" ;

   sdbCollection cl2 ;
   rc = cs5.getCollection( clName,cl2,false );
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cl" ;

   sdbCollection cl3 ;
   rc = cs5.getCollection( clName1,cl3,true );
   ASSERT_EQ( SDB_DMS_NOTEXIST, rc ) << "fail to get cl" ;

   sdbCollection cl4 ;
   rc = cs5.getCollection( clName1,cl4 );
   ASSERT_EQ( SDB_DMS_NOTEXIST, rc ) << "fail to get cl" ;

   sdbCollection cl5 ;
   rc = cs5.getCollection( clName,cl5);
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cl" ;
}