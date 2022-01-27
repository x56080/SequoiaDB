#include <stdio.h>
#include <gtest/gtest.h>
#include "arguments.hpp"
#include "testcommon.hpp"
#include "testBase.hpp"
#include "client.h"

class getDomainName25084: public testBase
{
protected:
   const CHAR* pDomainName        = "domain1" ;
   const CHAR* csName             = "getDomainName_25083" ;
   INT32 rc                       = SDB_OK ;
   sdbCSHandle collectionspace    = 0 ;
   sdbDomainHandle    dom         = 0 ;
   CHAR pResult[ NAME_LEN+1 ]     = { 0 } ;
   bson csOption ;
   bson domObj ;
   void SetUp()
   {
     testBase::SetUp() ;
     std::vector<std::string> groupNames ;
     rc = getGroups( db, groupNames ) ;
     ASSERT_EQ( SDB_OK, rc ) << "fail to get all groups " ;

     // Domain option bson
     bson_init( &domObj ) ;
     bson_append_start_array( &domObj, "Groups" ) ;
     bson_append_string( &domObj, "0", groupNames[0].c_str() ) ;
     bson_append_finish_array( &domObj ) ;
     bson_finish( &domObj ) ;

     // Create domain
     rc = sdbCreateDomain( db, pDomainName, &domObj, &dom ) ;
     ASSERT_EQ( SDB_OK, rc ) << "Failed to create domain, rc = " << rc ;

     // sdbCreateCollectionSpaceV2 option bson
     bson_init( &csOption ) ;
     bson_append_string( &csOption, "Domain", pDomainName ) ;
     bson_finish( &csOption ) ;
   }
   void TearDown()
   {
     //clear the environment
     rc = sdbDropDomain( db, pDomainName ) ;
     ASSERT_EQ( SDB_OK, rc ) ;
     bson_destroy( &csOption ) ;
     bson_destroy( &domObj ) ;
     sdbReleaseCS ( collectionspace ) ;
     sdbReleaseDomain ( dom ) ;
     testBase::TearDown() ;
   }
} ;

TEST_F(getDomainName25084, sdbCSGetDomainName_25084)
{
   //case 1:domain does not exists
   rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) << "Failed to create collectionspace, rc = " << rc;
   rc = sdbCSGetDomainName ( collectionspace, pResult, NAME_LEN ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( '\0', pResult[0] ) ;
   rc = sdbDropCollectionSpace ( db, csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "Failed to drop collection, rc = " << rc;

   //case 2:domain exists
   sdbReleaseCS ( collectionspace ) ;
   rc = sdbCreateCollectionSpaceV2( db, csName, &csOption, &collectionspace ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
   // get domain name
   rc = sdbCSGetDomainName ( collectionspace, pResult, NAME_LEN ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 0, strcmp( pResult, pDomainName ) ) ;
   rc = sdbDropCollectionSpace ( db, csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "Failed to drop collection, rc = " << rc;
}

