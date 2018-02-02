/**************************************************************
 * @Description: test lob func para
 *               seqDB-14338 : write/read/closeLob参数校验
 *               seqDB-14339:lock/seek/lockAndSeek参数校验
 *               seqDB-14340:getLobSize/CreateTime/modTime参数校验
 * @Modify:      Liang xuewang Init
 *               2018-02-02
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include "testcommon.hpp"

const CHAR* csName = "lobFuncTestCs14388" ;
const CHAR* clName = "lobFuncTestCl14388" ;
sdbConnectionHandle db = SDB_INVALID_HANDLE ;
sdbCSHandle cs = SDB_INVALID_HANDLE ;
sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
bson_oid_t oid ;

INT32 setup()
{
   INT32 rc = SDB_OK ;
   bson_oid_gen( &oid ) ;
   sdbLobHandle lob = SDB_INVALID_HANDLE ;
   const CHAR* buf = "ABCDEabcde" ;
   UINT32 len = strlen( buf ) ;

   rc = createNormalCl( &db, &cs, &cl, csName, clName ) ;
   CHECK_RC( rc, "fail to create cs %s, cl %s\n", csName, clName ) ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   CHECK_RC( rc, "fail to open lob\n" ) ;
   rc = sdbWriteLob( lob, buf, len ) ;
    CHECK_RC( rc, "fail to write lob\n" ) ;
   rc = sdbCloseLob( &lob ) ;
   CHECK_RC( rc, "fail to close lob\n" ) ;

done:
   return rc ;
error:
   goto done ;
}

INT32 teardown()
{
   INT32 rc = SDB_OK ;
   rc = sdbDropCollectionSpace( db, csName ) ;
   CHECK_RC( rc, "fail to drop cs %s\n", csName ) ;
done:
   sdbDisconnect( db ) ;
   sdbReleaseCollection( cl ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseConnection( db ) ;
   return rc ;
error:
   goto done ;
}

TEST( lobFunc, getSize )
{
   INT32 rc = SDB_OK ;
   rc = setup() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   SINT64 size ;
   rc = sdbGetLobSize( NULL, &size ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetLobSize( SDB_INVALID_HANDLE, &size ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetLobSize( cs, &size ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;

   sdbLobHandle lob = SDB_INVALID_HANDLE ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetLobSize( lob, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = teardown() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST( lobFunc, getCreateTime )
{
   INT32 rc = SDB_OK ;
   rc = setup() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   UINT64 millis ;
   rc = sdbGetLobCreateTime( NULL, &millis ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetLobCreateTime( SDB_INVALID_HANDLE, &millis ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetLobCreateTime( cs, &millis ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;

   sdbLobHandle lob = SDB_INVALID_HANDLE ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbGetLobCreateTime( lob, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = teardown() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}
