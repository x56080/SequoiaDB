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

   Source File Name = optAPMTest.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "optAPM.hpp"
#include "../testDef.h"
#include "rtnObjectStatInfo.hpp"
#include "dmsObjectMetaInfo.hpp"
#include <gtest/gtest.h>

using namespace engine;

RTN_INDEX_STAT_PTR buildRtnIndexStatInfo( const std::shared_ptr< const ossPoolString > &clFullName,
                                          const std::shared_ptr< const ossPoolString > &indexName,
                                          UINT32 indexPages,
                                          UINT32 indexLevels,
                                          BOOLEAN isUnique,
                                          const BSONObj &keyPattern )
{
   RTN_INDEX_STAT_PTR stat = makeSharedPtrFromPool< rtnIndexStatInfo >( clFullName, indexName );
   stat->setIndexPages( indexPages );
   stat->setIndexLevels( indexLevels );
   stat->setUnique( isUnique );
   stat->setKeyPattern( keyPattern );
   constexpr UINT32 SAMPLE_SIZE = 200;
   for ( UINT32 i = 0; i < SAMPLE_SIZE; ++i )
   {
      stat->pushMCVSet( BSON( "a" << i ), 1.0 / SAMPLE_SIZE );
   }
   stat->postInit();
   return stat;
}

constexpr UINT32 BUCKET_NUM = 128;
constexpr UINT32 SORT_BUFFER_SIZE = 256;
constexpr INT32 OPT_COST_THRESHOLD = 20;
constexpr BOOLEAN ENABLE_MIX_CMP = FALSE;

constexpr const CHAR *SAMPLE_CL_NAME_1 = "cs.cl1";
const std::shared_ptr< ossPoolString > SAMPLE_CL_1_NAME_PTR =
   makeSharedPtrFromPool< ossPoolString >( SAMPLE_CL_NAME_1 );
const utilCLUniqueID SAMPLE_CL_UID_1 = utilBuildCLUniqueID( 1, 1 );
const DMS_CL_META_PTR SAMPLE_CL_1_META =
   makeSharedPtrFromPool< dmsCollectionMetaInfo >( SAMPLE_CL_1_NAME_PTR,
                                                   1,
                                                   0,
                                                   16,
                                                   1284,
                                                   DPS_MAX_TRANS_TIME );
const RTN_CL_STAT_PTR SAMPLE_CL_1_STAT =
   makeSharedPtrFromPool< rtnCollectionStatInfo >( SAMPLE_CL_1_NAME_PTR,
                                                   1,
                                                   200,
                                                   60000,
                                                   1284,
                                                   65929411,
                                                   10 );
constexpr const CHAR *SAMPLE_CL_1_INDEX_1_NAME = "index1";
const std::shared_ptr< ossPoolString > SAMPLE_CL_1_INDEX_1_NAME_PTR =
   makeSharedPtrFromPool< ossPoolString >( SAMPLE_CL_1_INDEX_1_NAME );
const OID SAMPLE_CL_1_INDEX_1_OID = OID::gen();
const BSONObj SAMPLE_CL_1_INDEX_1_PATTERN = BSON( "age" << 1 );
const DMS_INDEX_META_PTR SAMPLE_CL_1_INDEX_1_META =
   makeSharedPtrFromPool< dmsIndexMetaInfo >( SAMPLE_CL_1_NAME_PTR,
                                              SAMPLE_CL_1_INDEX_1_NAME_PTR,
                                              SAMPLE_CL_UID_1,
                                              1,
                                              SAMPLE_CL_1_INDEX_1_OID,
                                              SAMPLE_CL_1_INDEX_1_PATTERN,
                                              1,
                                              1,
                                              1,
                                              0,
                                              0,
                                              FALSE,
                                              FALSE,
                                              FALSE,
                                              FALSE,
                                              FALSE );
const RTN_INDEX_STAT_PTR SAMPLE_CL_1_INDEX_1_STAT =
   buildRtnIndexStatInfo( SAMPLE_CL_1_NAME_PTR,
                          SAMPLE_CL_1_INDEX_1_NAME_PTR,
                          256,
                          2,
                          FALSE,
                          SAMPLE_CL_1_INDEX_1_PATTERN );

constexpr const CHAR *SAMPLE_CL_1_INDEX_2_NAME = "index2";
const std::shared_ptr< ossPoolString > SAMPLE_CL_1_INDEX_2_NAME_PTR =
   makeSharedPtrFromPool< ossPoolString >( SAMPLE_CL_1_INDEX_2_NAME );
const OID SAMPLE_CL_1_INDEX_2_OID = OID::gen();
const BSONObj SAMPLE_CL_1_INDEX_2_PATTERN = BSON( "age" << -1 );
const DMS_INDEX_META_PTR SAMPLE_CL_1_INDEX_2_META =
   makeSharedPtrFromPool< dmsIndexMetaInfo >( SAMPLE_CL_1_NAME_PTR,
                                              SAMPLE_CL_1_INDEX_2_NAME_PTR,
                                              SAMPLE_CL_UID_1,
                                              2,
                                              SAMPLE_CL_1_INDEX_2_OID,
                                              SAMPLE_CL_1_INDEX_2_PATTERN,
                                              1,
                                              1,
                                              1,
                                              0,
                                              0,
                                              FALSE,
                                              FALSE,
                                              FALSE,
                                              FALSE,
                                              FALSE );
const RTN_INDEX_STAT_PTR SAMPLE_CL_1_INDEX_2_STAT =
   buildRtnIndexStatInfo( SAMPLE_CL_1_NAME_PTR,
                          SAMPLE_CL_1_INDEX_2_NAME_PTR,
                          256,
                          2,
                          FALSE,
                          SAMPLE_CL_1_INDEX_2_PATTERN );

class opt_apm_test : public testing::Test
{
public:
   static void SetUpTestCase()
   {
      sdbEnablePD( "/opt/diaglog/sdb.log", 1, 1000 );
      setPDLevel( PDDEBUG );
      SAMPLE_CL_1_META->pushIndexMetaInfo( SAMPLE_CL_1_INDEX_1_META );
      SAMPLE_CL_1_META->pushIndexMetaInfo( SAMPLE_CL_1_INDEX_2_META );
      SAMPLE_CL_1_STAT->getIndexStatVec().push_back( SAMPLE_CL_1_INDEX_1_STAT );
      SAMPLE_CL_1_STAT->getIndexStatVec().push_back( SAMPLE_CL_1_INDEX_2_STAT );
   }

   static void TearDownTestCase() {}

   virtual void SetUp() override {}

   virtual void TearDown() override {}
};

TEST_F( opt_apm_test, base_get )
{
   INT32 rc = SDB_OK;
   BSONObj query = BSON( "age" << BSON( "gt" << 50 ) );
   BSONObj selector;
   BSONObj hint;
   {
      testExecutor executor;
      BSONObj orderBy = BSON( "age" << 1 );
      rtnQueryOptions options( query, selector, orderBy, hint, SAMPLE_CL_NAME_1, 0, 0, 0 );
      optAccessPlanRuntime planRuntime;
      optAccessPlanManager apm;
      rc = apm.init( BUCKET_NUM, OPT_PLAN_PARAMETERIZED, SORT_BUFFER_SIZE, OPT_COST_THRESHOLD,
                     ENABLE_MIX_CMP, FALSE );
      ASSERT_EQ( SDB_OK, rc );
      rc = apm.getAccessPlan(
         &executor, options, rtnCollectionInfo( SAMPLE_CL_1_META, SAMPLE_CL_1_STAT ), planRuntime );
      ASSERT_EQ( SDB_OK, rc );
      EXPECT_EQ( IXSCAN, planRuntime.getScanType() );
      EXPECT_STREQ( SAMPLE_CL_1_INDEX_1_NAME, planRuntime.getIndexName() );
      apm.fini();
   }
}

TEST_F( opt_apm_test, base_get_hint )
{
   INT32 rc = SDB_OK;
   BSONObj query = BSON( "age" << BSON( "gt" << 50 ) );
   BSONObj selector;
   BSONObj orderBy = BSON( "age" << 1 );
   BSONObj hint = BSON( "" << SAMPLE_CL_1_INDEX_2_NAME );
   testExecutor executor;
   rtnQueryOptions options( query, selector, orderBy, hint, SAMPLE_CL_NAME_1, 0, 0, 0 );
   optAccessPlanRuntime planRuntime;
   optAccessPlanManager apm;
   rc = apm.init( BUCKET_NUM, OPT_PLAN_PARAMETERIZED, SORT_BUFFER_SIZE, OPT_COST_THRESHOLD,
                  ENABLE_MIX_CMP, FALSE );
   ASSERT_EQ( SDB_OK, rc );
   rc = apm.getAccessPlan( &executor, options,
                           rtnCollectionInfo( SAMPLE_CL_1_META, SAMPLE_CL_1_STAT ), planRuntime );
   ASSERT_EQ( SDB_OK, rc );
   EXPECT_EQ( IXSCAN, planRuntime.getScanType() );
   EXPECT_STREQ( SAMPLE_CL_1_INDEX_2_NAME, planRuntime.getIndexName() );
   apm.fini();
}

TEST_F( opt_apm_test, base_get_temp )
{
   INT32 rc = SDB_OK;
   BSONObj query = BSON( "age" << BSON( "gt" << 50 ) );
   BSONObj selector;
   BSONObj hint;
   {
      testExecutor executor;
      BSONObj orderBy = BSON( "age" << 1 );
      rtnQueryOptions options( query, selector, orderBy, hint, SAMPLE_CL_NAME_1, 0, 0, 0 );
      optAccessPlanRuntime planRuntime;
      optAccessPlanManager apm;
      rc = apm.init( BUCKET_NUM, OPT_PLAN_PARAMETERIZED, SORT_BUFFER_SIZE, OPT_COST_THRESHOLD,
                     ENABLE_MIX_CMP, FALSE );
      ASSERT_EQ( SDB_OK, rc );
      rc = apm.getTempAccessPlan(
         &executor, options, rtnCollectionInfo( SAMPLE_CL_1_META, SAMPLE_CL_1_STAT ), planRuntime );
      ASSERT_EQ( SDB_OK, rc );
      ASSERT_EQ( TRUE, planRuntime.isNewPlan() );
      EXPECT_EQ( IXSCAN, planRuntime.getScanType() );
      EXPECT_STREQ( SAMPLE_CL_1_INDEX_1_NAME, planRuntime.getIndexName() );
      apm.fini();
   }
}