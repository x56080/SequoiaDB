/**************************************************************
 * @Description: test base class
 * @Modify     : Liang xuewang 
 *               2017-09-17
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include "arguments.hpp"
#include "testBase.hpp"

using namespace sdbclient ;

void testBase::SetUp()
{
   INT32 rc = SDB_OK ;
   rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb" ;
}

void testBase::TearDown()
{
   db.disconnect() ;
}

BOOLEAN testBase::shouldClear()
{
   return ( !HasFailure() || ARGS->forceClear() ) ;
}
