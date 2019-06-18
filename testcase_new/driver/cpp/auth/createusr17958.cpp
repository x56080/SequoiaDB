/**************************************************************
 * @Description: test createUsr()
 *               seqDB-17958 : createUsr接口支持用户级审计日志 
 * @Modify     : wenjing wang
 *               2019-05-31
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>

#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"


const char* usrName = "usr_17958" ;
const char* passwd = "usr_17958" ;

class createUsrTest17958 : public testBase 
{
protected:
   void SetUp()
   {
      INT32 rc = SDB_OK ;
      testBase::SetUp() ;
      isStandAlone = isStandalone ( db ) ; 
      if ( isStandAlone )
      {
         return ;
      }
      sdbReplicaGroup cataLog ;
      
      rc = db.getReplicaGroup( 1, cataLog )  ;
      ASSERT_EQ( SDB_OK, rc ) ; 
      
      sdbNode masterNode ;
      rc = cataLog.getMaster( masterNode ) ;
      ASSERT_EQ( SDB_OK, rc ) ; 
      
      rc = masterNode.connect( catalogDb ) ;
   }
   
   void TearDown()
   {
      if ( !isStandAlone )
      {
         db.removeUsr( usrName, passwd ) ;
      }
      testBase::TearDown() ;
   }
protected:
   BOOLEAN isStandAlone ;
   sdbclient::sdb catalogDb ;
} ;

TEST_F( createUsrTest17958, WithOption )
{  
   if ( isStandAlone )
   {
      return ;
   }
   const char* mask = "DML|DDL|DCL" ;
   INT32 rc = db.createUsr(usrName, usrName, BSON("AuditMask" << mask )) ;
   ASSERT_EQ( SDB_OK, rc ) ; 
   
   sdbCollection cl ;
   rc = catalogDb.getCollection( "SYSAUTH.SYSUSRS", cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ; 
   
   bson::BSONObj ret ;
   
   rc = cl.queryOne( ret, BSON("User" << usrName) ) ;
   ASSERT_EQ( SDB_OK, rc ) ; 
  
   bson::BSONObj sub = ret.getObjectField("Options") ; 
   ASSERT_EQ( strcmp(sub.getStringField("AuditMask"), mask), 0 ) ;
}

