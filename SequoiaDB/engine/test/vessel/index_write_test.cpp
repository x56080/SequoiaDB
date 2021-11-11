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
                                const CHAR *csName,
                                const CHAR *clName,
                                UINT32 count)
{
   test_executor session;
   bson::BSONObjBuilder builder;
   collectionHandler handler;
   INT32 rc = db->openCollection(&session, csName, clName, openCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);
   CHAR suffix[128] = {};
   ossMemset(suffix, 'a', sizeof(suffix) - 1);

   for (UINT32 i = 0; i < count; ++i)
   {
      CHAR pad[16 + sizeof(suffix)] = {0};
      ossItoa(ossRand(), pad, 10);
      builder.reset();
      builder.append("a", ossStrncat(pad, suffix, sizeof(pad) - 1));
      bson::BSONObj obj = builder.done();
      slice record;
      record.reset(obj.objsize(), obj.objdata());
      utilInsertResult res;
      rc = handler.insert(&session, record,
                          INVALID_STRIPING_ID,
                          insertOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);
   }
   handler.close();
}

static void insert_test_nonunique_index(INDEX_TYPE type)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   options.ioWorkerCount = 4;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   options.cacheOptions.flush.flushDirtyListThreshold = 0.8;
   options.cacheOptions.freelist.maxChunkCount = 1024;
   collectionHandler handler;
   UINT32 count = 10000000;
   static const UINT32 threadCount = 6;
   std::thread threads[threadCount];
   UINT32 countPerThread = count / threadCount;

   createCSOptions csOptions;
   csOptions.dataSegSize = STORAGE_FILE_SEGMENT_SIZE_32MB;

   closeDBOptions co;
   co.closeMode = closeDBOptions::CLOSE_MODE_IMMDIETE;

   collectionHandler clHandler;

   indexParameters params;
   params.type = type;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar1", 1, createCLOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCollection(&session, "foo", "bar1", openCLOptions(), clHandler);
   ASSERT_EQ(SDB_OK, rc);

   rc = clHandler.createIndex(&session, strSlice("index1"),
                              BSON("a" << 1), params, createIndexOptions());
   ASSERT_EQ(SDB_OK, rc);                   

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(thread_insert_index, &db,
                                         "foo", "bar1", countPerThread));
   }

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i].join();
   }

   rc = db.close(&session, co);
   ASSERT_EQ(SDB_OK, rc);
}

TEST_F(index_write_test, test1_1)
{
   insert_test_nonunique_index(INDEX_TYPE_BTREE);
}