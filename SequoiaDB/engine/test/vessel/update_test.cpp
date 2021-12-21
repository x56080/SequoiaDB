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

   Source File Name = update_test.cpp

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
#include "vessel/requestContext.h"
#include <gtest/gtest.h>
#include "ossUtil.hpp"
#include "vessel/IRedoLogger.h"
#include "../bson/bson.hpp"
#include "pd.hpp"
#include "vessel/builtinRecordUpdater.h"
#include "dmsCursorReader.hpp"

#include "mthMatchTree.hpp"
#include "interface/IDataStorageEngine.h"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;


class update_test : public testing::Test
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

TEST_F(update_test, base_update_test1)
{
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor executor;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   DATA_COLLECTION_PTR handler;
   INT32 rc = SDB_OK;
   bson::BSONObjBuilder builder;
   UINT32 count = 10000;
   ossPoolVector<dmsRecordID> rids;
   bson::BSONObj pattern = BSON("$inc" << BSON("a" << 1));
   bsonRecordUpdater updater;
   DATA_CURSOR_PTR cursor;

   rc = updater.init(pattern);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&executor, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&executor, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult insertRes;
      /// insert a as even number.
      builder.append("a", i * 2);
      builder.append("b", i);
      bson::BSONObj obj = builder.done();
      insertRes.enableReturnIDInfo();
      rc = handler->insertRecord(&executor, obj, dmsInsertRecordOptions(), &insertRes);
      ASSERT_EQ(SDB_OK, rc);

      INT32 page, slot;
      insertRes.getInsertLoc(page, slot);
      dmsRecordID rid(page, slot);
      rids.push_back(rid);
      builder.reset();
   }

   for (UINT32 i = 0; i < count; ++i)
   {
      /// update a to odd number.
      utilUpdateResult updateRes;
      rc = handler->updateRecord(&executor, rids[i], &updater,
                                 dmsUpdateRecordOptions(), &updateRes);
      ASSERT_EQ(SDB_OK, rc);
   }

   rc = handler->scan(&executor, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   dmsBsonCursorReader reader;
   reader.init(cursor, FALSE);
   for (UINT32 i = 0; i < count; ++i)
   {
      rc = reader.fetchNext(&executor);
      ASSERT_EQ(SDB_OK, rc);
      const bson::BSONObj  &r = reader.getRecord();
      ASSERT_EQ(r.getIntField("a"), r.getIntField("b") * 2 + 1);
   }

   reader.fini();
   db.close(&executor, closeDBOptions());

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   rc = handler->scan(&executor, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   reader.init(cursor, FALSE);
   for (UINT32 i = 0; i < count; ++i)
   {
      rc = reader.fetchNext(&executor);
      ASSERT_EQ(SDB_OK, rc);
      const bson::BSONObj  &r = reader.getRecord();
      ASSERT_EQ(r.getIntField("a"), r.getIntField("b") * 2 + 1);
   }

   db.close(&executor, closeDBOptions());
}

/// update with index
TEST_F(update_test, base_update_test2)
{
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor executor;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   DATA_COLLECTION_PTR handler;
   INT32 rc = SDB_OK;
   bson::BSONObjBuilder builder;
   UINT32 count = 10000;
   ossPoolVector<dmsRecordID> rids;
   bson::BSONObj pattern = BSON("$inc" << BSON("a" << 1));
   bsonRecordUpdater updater;
   DATA_CURSOR_PTR cursor;
   indexParameters params;
   params.type = INDEX_TYPE_BTREE;
   bson::BSONObj indexDef = indexTestUtil::createIndexObj("index", params, BSON("a" << 1));

   rc = updater.init(pattern);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&executor, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&executor, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   rc = handler->createIndex(&executor, dmsBuildIndexOptions(), indexDef);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult insertRes;
      /// insert a as even number.
      builder.append("a", i * 2);
      builder.append("b", i);
      bson::BSONObj obj = builder.done();
      insertRes.enableReturnIDInfo();
      rc = handler->insertRecord(&executor, obj, dmsInsertRecordOptions(), &insertRes);
      ASSERT_EQ(SDB_OK, rc);

      INT32 page, slot;
      insertRes.getInsertLoc(page, slot);
      dmsRecordID rid(page, slot);
      rids.push_back(rid);
      builder.reset();
   }

   for (UINT32 i = 0; i < count; ++i)
   {
      /// update a to odd number.
      utilUpdateResult updateRes;
      rc = handler->updateRecord(&executor, rids[i],
                                 &updater, dmsUpdateRecordOptions(), &updateRes);
      ASSERT_EQ(SDB_OK, rc);
   }

   rc = handler->scan(&executor, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   dmsBsonCursorReader reader;
   reader.init(cursor, FALSE);
   for (UINT32 i = 0; i < count; ++i)
   {
      rc = reader.fetchNext(&executor);
      ASSERT_EQ(SDB_OK, rc);
      const bson::BSONObj &r = reader.getRecord();
      ASSERT_EQ(r.getIntField("a"), r.getIntField("b") * 2 + 1);
   }

   reader.fini();
   handler->close();
   db.close(&executor, closeDBOptions());

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   rc = handler->scan(&executor, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   reader.init(cursor, FALSE);
   for (UINT32 i = 0; i < count; ++i)
   {
      rc = reader.fetchNext(&executor);
      ASSERT_EQ(SDB_OK, rc);
      const bson::BSONObj &r = reader.getRecord();
      ASSERT_EQ(r.getIntField("a"), r.getIntField("b") * 2 + 1);
   }

   reader.fini();
   handler->close();
   db.close(&executor, closeDBOptions());
}