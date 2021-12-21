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

   INT32 rc = SDB_OK;
   bson::BSONObjBuilder builder;
   UINT32 count = 10000;
   ossPoolVector<dmsRecordID> rids;

   DATA_COLLECTION_PTR cl;

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&executor, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&executor, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), cl);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      utilInsertResult insertRes;
      /// insert a as even number.
      builder.append("a", i);
      builder.append("b", i);

      insertRes.enableReturnIDInfo();
      rc = cl->insertRecord(&executor, builder.done(),
                            dmsInsertRecordOptions(), &insertRes);
      ASSERT_EQ(SDB_OK, rc);

      INT32 page, slot;
      insertRes.getInsertLoc(page, slot);
      dmsRecordID rid(page, slot);
      rids.push_back(rid);
      builder.reset();
   }

   for (UINT32 i = 0; i < count; ++i)
   {

      utilDeleteResult deleteRes;
      const dmsRecordID &rid = rids[i];
      rc = cl->deleteRecord(&executor, rid, dmsDeleteRecordOptions(), &deleteRes);
      ASSERT_EQ(SDB_OK, rc);
   }

   UINT64 currentCount = 0;
   rc = cl->getRecordCount(&executor, currentCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, currentCount);

   DATA_CURSOR_PTR cursor;
   rc = cl->scan(&executor, dmsScanOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);

   rc = cursor->fetchNext(&executor);
   ASSERT_EQ(SDB_DMS_EOC, rc);

   db.close(&executor, closeDBOptions());

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), cl);
   ASSERT_EQ(SDB_OK, rc);

   rc = cl->getRecordCount(&executor, currentCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, currentCount);

   db.close(&executor, closeDBOptions());
}