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

   Source File Name = lsm_insert_test.cpp

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
#include "dpsLogRecord.hpp"
#include "vessel/listCollectionSpaceDef.h"
#include "vessel/listCollectionsDef.h"
#include "vessel/logRecordContext.h"
#include "dpsLogRecord.hpp"
#include "../bson/bson.hpp"
#include "pd.hpp"
#include "ixmKey.hpp"
#include "dpsTransID.hpp"
#include "vessel/lsm/lsmDB.hpp"
#include "vessel/lsm/lsmIndex.hpp"
#include "vessel/lsm/lsmIndexMeta.hpp"
#include "dms.hpp"
#include "vessel/globalIndexID.h"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

class lsm_insert_test : public testing::Test
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

void thread_insert(lsmDB *db, UINT32 count)
{
   INT32 rc = SDB_OK;
   ::engine::vessel::globalIndexID gid(0, 0, 0);
   ::engine::vessel::orderingWrapper ordering;
   ::engine::vessel::lsmIndexMeta meta(gid, ordering);
   DPS_TRANS_ID transID;
   bson::BSONObjBuilder builder;
   ::engine::vessel::lsmIndex lsm;


   rc = lsm.init(db, meta);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      builder.reset();
      builder.append("a", i);
      bson::BSONObj obj = builder.done();
      ::engine::ixmKeyOwned key(obj);
      lsmKeyEntry entry;
      ::engine::vessel::recordID rid(i, i);
      entry.shallowCopy(key, rid, i, transID);
      rc = lsm.keyInsert(entry);
      ASSERT_EQ(SDB_OK, rc);
   }
}

TEST_F(lsm_insert_test, DISABLED_test1)
{
   rocksdb::Status status;
   LSMConfig conf;
   conf.dbPath = LSM_PATH;
   conf.createDBIfMissing = TRUE;
   lsmDB db;
   db.initLsmDB(conf);
   status = db.openLsmDB();
   ASSERT_TRUE(status.ok());

   static const UINT32 THREAD_COUNT = 6;
   UINT32 recordCount = 10000000;

   std::thread threads[THREAD_COUNT];
   for (UINT32 i = 0; i < THREAD_COUNT; ++i)
   {
      threads[i] = std::move(std::thread(thread_insert, &db, recordCount/THREAD_COUNT));
   }

   for (UINT32 i = 0; i < THREAD_COUNT; ++i)
   {
      threads[i].join();
   }

   status = db.closeLsmDB(FALSE, TRUE);
   ASSERT_TRUE(status.ok());
}