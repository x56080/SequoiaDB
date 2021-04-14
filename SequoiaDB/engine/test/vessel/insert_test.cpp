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

   Source File Name = insert_test.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

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
#include "vessel/ISession.h"
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

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;


class insert_test : public testing::Test
{
   public:
   static void SetUpTestCase()
   {
      fs::path testPath(DATA_PATH);
      fs::remove_all(testPath);
      fs::create_directory(testPath);
   }

   static void TearDownTestCase()
   {
      fs::path testPath(DATA_PATH);
      fs::remove_all(testPath);
   }

   virtual void SetUp()
   {
      fs::path testPath(DATA_PATH);
      fs::remove_all(testPath);
      fs::create_directory(testPath);
   }
};

TEST_F(insert_test, test1)
{
   INT32 rc = SDB_OK;
   test_logger logger;
   vesselImpl db;
   outerResource resource;
   resource.logger = &logger; 
   test_session session;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   collectionHandler handler;
   recordData record;
   DPS_TRANS_ID transID;
   utilInsertResult res;
   CHAR pad[1024] = {0};
   bson::BSONObjBuilder builder;
   builder.append("a", 1);
   builder.append("b", 2);
   builder.append("c", pad, 1024);
   bson::BSONObj obj = builder.obj();
   record.reset(RECORD_DATA_TYPE_BSON, slice(obj.objsize(), obj.objdata()));
   UINT32 count = 100;
   UINT64 recordCount = 0;
   slice recordSlice;

   cursorHandler cursor;

   db.initOuterResource(resource);
   rc = db.open(&session, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, createCSOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar1", 1, createCLOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCollection(&session, "foo", "bar1", openCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < 100; ++i)
   {
      rc = handler.insert(&session, record, transID, INVALID_STRIPING_ID, NULL, res);
      ASSERT_EQ(SDB_OK, rc);
   }

   rc = handler.getRecordCount(&session, NULL, recordCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(count, recordCount);
   
   rc = handler.openScanCursor(&session, NULL, NULL, NULL, cursor);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      const CHAR *raw = NULL;
      recordSlice.reset();
      rc = cursor.getNext(&session, recordSlice);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_LE(16, recordSlice.len());
      raw = recordSlice.data() + 16;
      bson::BSONObj obj(raw);
      ASSERT_EQ(1, obj.getIntField("a"));
      ASSERT_EQ(2, obj.getIntField("b"));
   }
   rc = cursor.getNext(&session, recordSlice);
   ASSERT_EQ(SDB_VESSEL_END_OF_CURSOR, rc);

   cursor.close();
   handler.close();
   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

TEST_F(insert_test, test2)
{
   INT32 rc = SDB_OK;
   test_logger logger;
   vesselImpl db;
   outerResource resource;
   resource.logger = &logger; 
   test_session session;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   collectionHandler handler;
   recordData record;
   DPS_TRANS_ID transID;
   utilInsertResult res;
   CHAR pad[1024] = {0};
   bson::BSONObjBuilder builder;
   builder.append("a", 1);
   builder.append("b", 2);
   builder.append("c", pad, 1024);
   bson::BSONObj obj = builder.obj();
   record.reset(RECORD_DATA_TYPE_BSON, slice(obj.objsize(), obj.objdata()));
   UINT32 count = 100;
   UINT64 recordCount = 0;
   slice recordSlice;

   cursorHandler cursor;

   db.initOuterResource(resource);
   rc = db.open(&session, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, createCSOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar1", 1, createCLOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCollection(&session, "foo", "bar1", openCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < 100; ++i)
   {
      rc = handler.insert(&session, record, transID, INVALID_STRIPING_ID, NULL, res);
      ASSERT_EQ(SDB_OK, rc);
   }

   rc = handler.getRecordCount(&session, NULL, recordCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(count, recordCount);
   
   rc = handler.openScanCursor(&session, NULL, NULL, NULL, cursor);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      const CHAR *raw = NULL;
      recordSlice.reset();
      rc = cursor.getNext(&session, recordSlice);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_LE(16, recordSlice.len());
      raw = recordSlice.data() + 16;
      bson::BSONObj obj(raw);
      ASSERT_EQ(1, obj.getIntField("a"));
      ASSERT_EQ(2, obj.getIntField("b"));
   }
   rc = cursor.getNext(&session, recordSlice);
   ASSERT_EQ(SDB_VESSEL_END_OF_CURSOR, rc);

   cursor.close();
   handler.close();
   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&session, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCollection(&session, "foo", "bar1", openCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

    rc = handler.getRecordCount(&session, NULL, recordCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(count, recordCount);

   rc = handler.openScanCursor(&session, NULL, NULL, NULL, cursor);
   ASSERT_EQ(SDB_OK, rc);

   handler.close();

   for (UINT32 i = 0; i < count; ++i)
   {
      const CHAR *raw = NULL;
      recordSlice.reset();
      rc = cursor.getNext(&session, recordSlice);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_LE(16, recordSlice.len());
      raw = recordSlice.data() + 16;
      bson::BSONObj obj(raw);
      ASSERT_EQ(1, obj.getIntField("a"));
      ASSERT_EQ(2, obj.getIntField("b"));
   }
   rc = cursor.getNext(&session, recordSlice);
   ASSERT_EQ(SDB_VESSEL_END_OF_CURSOR, rc);

   cursor.close();
   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}