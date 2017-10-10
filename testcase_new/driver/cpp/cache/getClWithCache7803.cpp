/**************************************************************
 * @Description: turn on cache and get cl
 *               seqDB-7803 : turn on cache and get cl(by fullName)
 * @Modify     : Suqiang Ling
 *               2017-09-11
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include <vector>
#include <sys/time.h>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"
#include "client.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class turnOnCache7803 : public testBase 
{
protected:
   const CHAR *pCsName ;
   const CHAR *pClName ;
   string clFullNameStr ;

   void SetUp() 
   {
      INT32 rc = SDB_OK ;

      // turn off cache
      sdbClientConf conf ;
      conf.enableCacheStrategy = FALSE ;
      rc = initClient( &conf );
      ASSERT_EQ( SDB_OK, rc ) << "fail to initClient" ;

      // connect db
      rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() );
      ASSERT_EQ( SDB_OK, rc ) << "fail to connect db" ;

      // create cs
      pCsName = "turnOnCache7803" ;
      sdbCollectionSpace cs ;
      rc = db.createCollectionSpace( pCsName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;

      // create cl
      pClName = "turnOnCache7803" ;
      sdbCollection cl ;
      rc = cs.createCollection( pClName, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ;

      // turn on cache
      conf.enableCacheStrategy = TRUE ;
      conf.cacheTimeInterval = 0 ;
      rc = initClient( &conf );
      ASSERT_EQ( SDB_OK, rc ) << "fail to initClient" ;

      clFullNameStr = string( pCsName ) + "." + string( pClName ) ;
   }

   void TearDown() 
   {
      if( shouldClear() )
      {
         INT32 rc = SDB_OK ;
         rc = db.dropCollectionSpace( pCsName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
      }
      db.disconnect() ;
   }

   INT32 getElapesdTimeOfGetCl( clock_t &elapsedUsec )
   {
      struct timeval begin, end ;
      sdbCollection cl ;
      INT32 rc ;

      gettimeofday( &begin, NULL ) ;
      rc = db.getCollection( clFullNameStr.c_str(), cl ) ;
      gettimeofday( &end, NULL ) ;
      
      elapsedUsec = ( end.tv_sec - begin.tv_sec ) * 1000000  + end.tv_usec - begin.tv_usec ;

      return rc ;
   }
} ;

TEST_F( turnOnCache7803, getCollection )
{
   INT32 rc = SDB_OK ;
   clock_t outCacheTime, inCacheTime ;

   rc = getElapesdTimeOfGetCl( outCacheTime ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getElapesdTimeOfGetCl( inCacheTime ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_LT( inCacheTime, outCacheTime );
}
