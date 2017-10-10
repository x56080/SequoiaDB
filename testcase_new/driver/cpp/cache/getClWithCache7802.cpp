/**************************************************************
 * @Description: turn on cache and get cl
 *               seqDB-7802 : turn on cache and get cl
 *               seqDB-7805 : turn on cache and get cl when 
 *                            it's cache time out
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

#define CACHE_TIME_INT 1000 /*millisecond*/

class turnOnCache7802 : public testBase 
{
protected:
   const CHAR *pCsName ;
   const CHAR *pClName ;
   sdbCollectionSpace cs ;

   void SetUp() 
   {
      INT32 rc = SDB_OK ;

      // turn off cache
      sdbClientConf conf ;
      conf.enableCacheStrategy = FALSE ;
      rc = initClient( &conf );
      ASSERT_EQ( SDB_OK, rc ) << "fail to initClient" ;

      // connect and create cs
      pCsName = "turnOnCache7802" ;
      pClName = "turnOnCache7802" ;
      sdbCollection cl ;
      rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() );
      ASSERT_EQ( SDB_OK, rc ) << "fail to connect db" ;
      rc = db.createCollectionSpace( pCsName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
      rc = cs.createCollection( pClName, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ;

      // turn on cache
      conf.enableCacheStrategy = TRUE ;
      conf.cacheTimeInterval = CACHE_TIME_INT / 1000 ;
      rc = initClient( &conf );
      ASSERT_EQ( SDB_OK, rc ) << "fail to initClient" ;
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
      rc = cs.getCollection( pClName, cl ) ;
      gettimeofday( &end, NULL ) ;
      
      elapsedUsec = ( end.tv_sec - begin.tv_sec ) * 1000000  + end.tv_usec - begin.tv_usec ;

      return rc ;
   }
} ;

TEST_F( turnOnCache7802, getCollection )
{
   INT32 rc = SDB_OK ;
   clock_t outCacheTime, inCacheTime ;
   rc = getElapesdTimeOfGetCl( outCacheTime ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getElapesdTimeOfGetCl( inCacheTime ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_LT( inCacheTime, outCacheTime );
   
   ossSleep( CACHE_TIME_INT ) ; // sleep utill cl not in cache 
   clock_t outCacheTime2 ;
   rc = getElapesdTimeOfGetCl( outCacheTime2 ) ; // seqDB-7805
   ASSERT_EQ( SDB_OK, rc ) ;

   ASSERT_LT( inCacheTime, outCacheTime2 );
}
