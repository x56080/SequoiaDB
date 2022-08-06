/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = index_write_test.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "test_def.h"
#include "vessel/vesselImpl.h"
#include <gtest/gtest.h>
#include <atomic>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

class index_write_test : public testing::Test
{
   public:
   static void SetUpTestCase()
   {
      {
      fs::path testPath(DATA_PATH);
      fs::remove_all(testPath);
      fs::create_directory(testPath);
      }
      {
      fs::path testPath(LSM_PATH);
      fs::remove_all(testPath);
      fs::create_directory(testPath);
      }
   }

   static void TearDownTestCase()
   {
      {
      fs::path testPath(DATA_PATH);
      fs::remove_all(testPath);
      }
      {
      fs::path testPath(LSM_PATH);
      fs::remove_all(testPath);
      }
   }

   virtual void SetUp()
   {
      {
      fs::path testPath(DATA_PATH);
      fs::remove_all(testPath);
      fs::create_directory(testPath);
      }
      {
      fs::path testPath(LSM_PATH);
      fs::remove_all(testPath);
      fs::create_directory(testPath);
      }
   }
};

static void thread_insert_index(vesselImpl *db,
                                const CHAR *fullName,
                                UINT32 count,
                                std::atomic_llong *counter)
{
   test_executor session;
   bson::BSONObjBuilder builder;
   DATA_COLLECTION_PTR handler;
   INT32 rc = db->openCL(&session, fullName, dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);
   CHAR pad[32] = {};
   ossMemset(pad, 'a', sizeof(pad) - 1);

   for (UINT32 i = 0; i < count; ++i)
   {
      for (UINT32 pos = 0; pos < 10; ++pos)
      {
         pad[pos] = 'a' + ossRand() % 52;
      }
      builder.reset();
      builder.append("a", pad);
      bson::BSONObj obj = builder.done();
      utilInsertResult res;
      rc = handler->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);
      counter->fetch_add(1, std::memory_order_relaxed);
   }
   handler->close();
}

static void insert_test_nonunique_index(INDEX_TYPE type)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   DATA_COLLECTION_PTR handler;
   UINT32 count = 10000000;
   static const UINT32 threadCount = 4;
   std::thread threads[threadCount];
   UINT32 countPerThread = count / threadCount;
   std::atomic_llong counter;

   closeDBOptions co;
   co.closeMode = closeDBOptions::CLOSE_MODE_IMMDIETE;

   bson::BSONObj indexDef = indexTestUtil::createIndexObj(type, "index1", 
                                                          FALSE, BSON("a" << 1));

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
   ASSERT_EQ(SDB_OK, rc);                   

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(thread_insert_index, &db,
                                         "foo.bar", countPerThread, &counter));
   }

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i].join();
   }

   rc = db.close(&session, co);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(counter.load(), count);
}

TEST_F(index_write_test, test1_1)
{
   insert_test_nonunique_index(INDEX_TYPE_HYBRID_TREE);
}

static void thread_insert_unique_index(vesselImpl *db,
                                 const CHAR *fullName,
                                 UINT32 count,
                                 UINT32 range,
                                 std::atomic_llong *counter)
{
   test_executor session;
   bson::BSONObjBuilder builder;
   DATA_COLLECTION_PTR handler;
   INT32 rc = db->openCL(&session, fullName, dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      builder.reset();
      builder.append("a", ossRand() % range);
      bson::BSONObj obj = builder.done();
      utilInsertResult res;
      rc = handler->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
      if (SDB_IXM_DUP_KEY == rc)
      {
         continue;
      }
      else if (SDB_OK == rc)
      {
         counter->fetch_add(1, std::memory_order_relaxed);
      }
      else
      {
         ASSERT_EQ(rc, SDB_OK);
      }
   }
   handler->close();
}

static void insert_test_unique_index(INDEX_TYPE type)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   DATA_COLLECTION_PTR handler;
   UINT32 count = 10000000;
   UINT32 range = 100;
   static const UINT32 threadCount = 4;
   std::thread threads[threadCount];
   UINT32 countPerThread = count / threadCount;
   std::atomic_llong counter;

   closeDBOptions co;
   co.closeMode = closeDBOptions::CLOSE_MODE_IMMDIETE;

   bson::BSONObj indexDef = indexTestUtil::createIndexObj(type, "index1", 
                                                          TRUE, BSON("a" << 1));

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
   ASSERT_EQ(SDB_OK, rc);                   

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(thread_insert_unique_index, &db,
                                         "foo.bar", countPerThread, range, &counter));
   }

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i].join();
   }

   rc = db.close(&session, co);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_LE(counter.load(), range);
}

/// unique index
TEST_F(index_write_test, test2)
{
   insert_test_unique_index(INDEX_TYPE_HYBRID_TREE);
}