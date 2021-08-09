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

   Source File Name = cs_ddl_test.cpp

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

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

class cs_ddl_test : public testing::Test
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


TEST_F(cs_ddl_test, test1)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource;
   resource.logger = test_logger::instance(); 
   resource.sessionMgr = test_session_mgr::instance();
   test_session session(test_logger::instance());
   openDBOptions options;
   createCSOptions csOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   cursorHandler cursor;

   UINT32 count = 0;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_OK, rc);
   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(1, count);

   rc = db.createCollectionSpace(&session, "foo", 2, csOptions);
   ASSERT_EQ(SDB_DMS_CS_EXIST, rc);

   rc = db.createCollectionSpace(&session, "foo1", 1, csOptions);
   ASSERT_EQ(SDB_DMS_CS_EXIST, rc);

   rc = db.createCollectionSpace(&session, "bar", 2, csOptions); 
   ASSERT_EQ(SDB_OK, rc);
   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(2, count);
   db.close(&session, closeDBOptions());
}


TEST_F(cs_ddl_test, test2)
{
   INT32 rc = SDB_OK;
   outerResource resource;
   resource.logger = test_logger::instance(); 
   resource.sessionMgr = test_session_mgr::instance();
   vesselImpl db;
   test_session session(test_logger::instance());
   openDBOptions options;
   createCSOptions csOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   UINT32 createdCount = 0;
   UINT32 count = 0;


   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < MAX_SU_COUNT; ++i)
   {
      CHAR buf[6] = {0};
      ossSnprintf(buf, 6, "%d", i);

      rc = db.createCollectionSpace(&session, buf, i, csOptions);
      if (SDB_OK == rc)
      {
         ++createdCount;
         continue;
      }
      else if (SDB_NOSPC == rc)
      {
         rc = SDB_OK;
         break;
      }
      else
      {
         ASSERT_TRUE(FALSE);
      }
   }

   if (MAX_SU_COUNT == createdCount)
   {
      CHAR buf[6] = {0};
      ossSnprintf(buf, 6, "%d", MAX_SU_COUNT);
      rc = db.createCollectionSpace(&session, buf, MAX_SU_COUNT, csOptions);
      ASSERT_EQ(SDB_DMS_SU_OUTRANGE, rc);
   }

   db.close(&session, closeDBOptions());

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(count, createdCount);
   db.close(&session, closeDBOptions());
}


TEST_F(cs_ddl_test, test3)
{
   INT32 rc = SDB_OK;
   outerResource resource;
   resource.logger = test_logger::instance(); 
   resource.sessionMgr = test_session_mgr::instance();
   vesselImpl db;
   test_session session(test_logger::instance());
   openDBOptions options;
   createCSOptions csOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   UINT32 count = 0;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "bar", 2, csOptions);
   ASSERT_EQ(SDB_OK, rc);
   db.close(&session, closeDBOptions());

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);
   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(2, count);
   db.close(&session, closeDBOptions());
}

TEST_F(cs_ddl_test, test4)
{
   INT32 rc = SDB_OK;

   outerResource resource;
   resource.logger = test_logger::instance(); 
   resource.sessionMgr = test_session_mgr::instance();
   vesselImpl db;
   test_session session(test_logger::instance());
   openDBOptions options;
   createCSOptions csOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   UINT32 count = 0;
   slice content;
   bson::BSONObj record;
   cursorHandler c;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, count);

   rc = db.listCollectionSpace(&session, NULL, c);
   ASSERT_EQ(SDB_OK, rc);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_VESSEL_EOC, rc);
   c.close();

   rc = db.createCollectionSpace(&session, "foo1", 1, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo2", 2, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo3", 3, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(3, count);

   rc = db.listCollectionSpace(&session, NULL, c);
   ASSERT_EQ(SDB_OK, rc);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.isValid());
   record = bson::BSONObj(content.data());
   ASSERT_EQ(0, ossStrcmp("foo1", record.getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(1, record.getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.isValid());
   record = bson::BSONObj(content.data());
   ASSERT_EQ(0, ossStrcmp("foo2", record.getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(2, record.getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.isValid());
   record = bson::BSONObj(content.data());
   ASSERT_EQ(0, ossStrcmp("foo3", record.getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(3, record.getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_VESSEL_EOC, rc);
   c.close();


   db.close(&session, closeDBOptions());

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);
   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(3, count);

   rc = db.listCollectionSpace(&session, NULL, c);
   ASSERT_EQ(SDB_OK, rc);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.isValid());
   record = bson::BSONObj(content.data());
   ASSERT_EQ(0, ossStrcmp("foo1", record.getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(1, record.getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.isValid());
   record = bson::BSONObj(content.data());
   ASSERT_EQ(0, ossStrcmp("foo2", record.getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(2, record.getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.isValid());
   record = bson::BSONObj(content.data());
   ASSERT_EQ(0, ossStrcmp("foo3", record.getStringField(CS_DUMP_RECORD_FIELD_NAME)));
   ASSERT_EQ(3, record.getIntField(CS_DUMP_RECORD_FIELD_UNIQUE_ID));

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_VESSEL_EOC, rc);
   c.close();

   db.close(&session, closeDBOptions());
}
