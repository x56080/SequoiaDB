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

   Source File Name = dml_delete_test.cpp

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
#include "vessel/dataScanRow.h"
#include "vessel/collectionOptions.h"
#include "vessel/builtinRecordUpdater.h"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;


class dml_delete_test : public testing::Test
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

TEST_F(dml_delete_test, base_delete_test1)
{
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor executor;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   collectionHandler handler;
   INT32 rc = SDB_OK;
   bson::BSONObjBuilder builder;
   UINT32 count = 10000;
   cursorHandler cursor;
   ossPoolVector<recordID> rids;
   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&executor, "foo", 1, createCSOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&executor, "foo", "bar", 1, createCLOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCollection(&executor, "foo", "bar", openCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult insertRes;
      /// insert a as even number.
      builder.append("a", i);
      builder.append("b", i);
      bson::BSONObj obj = builder.done();
      slice record(obj.objsize(), obj.objdata());
      insertRes.enableReturnIDInfo();
      rc = handler.insert(&executor, record,
                          INVALID_STRIPING_ID, insertOptions(), &insertRes);
      ASSERT_EQ(SDB_OK, rc);

      INT32 page, slot;
      insertRes.getInsertLoc(page, slot);
      recordID rid(page, slot);
      rids.push_back(rid);
      builder.reset();
   }

   for (UINT32 i = 0; i < count; ++i)
   {

      utilDeleteResult deleteRes;
      const recordID &rid = rids[i];
      dmlRemoveRequest request;
      request.rid = rid;
      rc = handler.deleteRecord(&executor, request, &deleteRes);
      ASSERT_EQ(SDB_OK, rc);
   }

   UINT64 currentCount = 0;
   rc = handler.getTotalRecordCountInPageHead(&executor, currentCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, currentCount);

   rc = handler.openScanCursor(&executor, NULL, collectionScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);
   dataScanRow row;
   rc = cursor.getNextRow(&executor, row);
   ASSERT_EQ(SDB_VESSEL_EOC, rc);

   db.close(&executor, closeDBOptions());

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCollection(&executor, "foo", "bar", openCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   rc = handler.getTotalRecordCountInPageHead(&executor, currentCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, currentCount);

   db.close(&executor, closeDBOptions());
}