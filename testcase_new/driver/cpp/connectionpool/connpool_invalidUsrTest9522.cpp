/*************************************************************
 * @Description: testcase for connectionpool
 *               seqDB-9522:提供不合法的_userName
 *               seqDB-9523:提供不合法的_passwd
 * @Modify:      Liangxw
 *               2019-09-05
 *************************************************************/
#include <sdbConnectionPool.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include "connpool_common.hpp"

using namespace std ;
using namespace sdbclient ;

class invalidUsrTest9522 : public testBase
{
protected:
   sdbConnectionPool ds ;
   sdbConnectionPoolConf conf ;
   string url ;
<<<<<<< HEAD

   void SetUp()
   {
=======
   BOOLEAN isReadyUser;
   void SetUp()
   {
      isReadyUser = FALSE ;
      testBase::SetUp();
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
      url = ARGS->coordUrl() ;
   }
   void TearDown()
   {
<<<<<<< HEAD
      ds.close() ;
=======
      if (isReadyUser)
      {
         db.removeUsr( "root", "sequoiadb" ) ;
      }
      ds.close() ;
      testBase::TearDown();
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   }
} ;

// 用户信息非法时，init正常返回, getConnection报错( 9522-9523 )
TEST_F( invalidUsrTest9522, userInfo9522 )
{
   INT32 rc = SDB_OK ;
   
   // init and get connection
   sdb* conn = NULL ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
   rc = ds.getConnection( conn ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;

   // check standalone
   if( isStandalone( *conn ) )
   {
      cout << "Run mode is standalone." << endl ;
      ds.releaseConnection( conn ) ;
      return ;
   }

   // create user
   rc = conn->createUsr( "root", "sequoiadb" ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to craete user" ;
<<<<<<< HEAD

=======
   isReadyUser = TRUE ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   // release connection and close
   ds.releaseConnection( conn ) ;
   ds.close() ;
	
   // test get connection with illegal username
   conf.setAuthInfo( "lxw", "sequoiadb" ) ;
   rc = ds.init( url, conf ) ;
<<<<<<< HEAD
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
   rc = ds.getConnection( conn ) ;
   ASSERT_EQ( SDB_AUTH_AUTHORITY_FORBIDDEN, rc ) << "fail to test get connection with invalid user" ;
=======
   ASSERT_EQ( SDB_AUTH_AUTHORITY_FORBIDDEN, rc ) << "fail to init connectionpool" ;
   //rc = ds.getConnection( conn ) ;
   //ASSERT_EQ( SDB_AUTH_AUTHORITY_FORBIDDEN, rc ) << "fail to test get connection with invalid user" ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   ds.close() ;
	
   // test get connection with no passwd
   conf.setAuthInfo( "root", "" ) ;
   rc = ds.init( url, conf ) ;
<<<<<<< HEAD
   ASSERT_EQ( SDB_OK, rc ) << "fail to test init connectionpool" ;
   rc = ds.getConnection( conn ) ;
   ASSERT_EQ( SDB_AUTH_AUTHORITY_FORBIDDEN, rc ) << "fail to test get connection with no passwd" ;
=======
   ASSERT_EQ( SDB_AUTH_AUTHORITY_FORBIDDEN, rc ) << "fail to test init connectionpool" ;
   //rc = ds.getConnection( conn ) ;
   //ASSERT_EQ( SDB_AUTH_AUTHORITY_FORBIDDEN, rc ) << "fail to test get connection with no passwd" ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   ds.close() ;
		
   // test get connection with illegal passwd
   conf.setAuthInfo( "root", "seq" ) ;
   rc = ds.init( url, conf ) ;
<<<<<<< HEAD
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
   rc = ds.getConnection( conn ) ;
   ASSERT_EQ( SDB_AUTH_AUTHORITY_FORBIDDEN, rc ) << "fail to test get connection with illegal passwd" ;
=======
   ASSERT_EQ( SDB_AUTH_AUTHORITY_FORBIDDEN, rc ) << "fail to init connectionpool" ;
   //rc = ds.getConnection( conn ) ;
   //ASSERT_EQ( SDB_AUTH_AUTHORITY_FORBIDDEN, rc ) << "fail to test get connection with illegal passwd" ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   ds.close() ;

   // test get connection with legal user and remove user
   conf.setAuthInfo( "root", "sequoiadb" ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;
   rc = ds.getConnection( conn ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;
   rc = conn->removeUsr( "root", "sequoiadb" ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to remove user" ;
<<<<<<< HEAD
=======
   isReadyUser = FALSE ;
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
   ds.releaseConnection( conn ) ;
}
