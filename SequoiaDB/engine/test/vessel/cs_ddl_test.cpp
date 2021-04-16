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
   test_logger logger;
   vesselImpl db;
   outerResource resource;
   resource.logger = &logger; 
   test_session session;
   openDBOptions options;
   createCSOptions csOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;

   utilCSUniqueID uniqueID;
   cursorHandler cursor;

   UINT32 count = 0;

   db.initOuterResource(resource);
   rc = db.open(&session, options);
   ASSERT_EQ(SDB_OK, rc);

   uniqueID = 1;
   rc = db.createCollectionSpace(&session, "foo", uniqueID, csOptions);
   ASSERT_EQ(SDB_OK, rc);
   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(1, count);

   uniqueID = 2;
   rc = db.createCollectionSpace(&session, "foo", uniqueID, csOptions);
   ASSERT_EQ(SDB_DMS_CS_EXIST, rc);

   uniqueID = 1;
   rc = db.createCollectionSpace(&session, "foo1", uniqueID, csOptions);
   ASSERT_EQ(SDB_DMS_CS_EXIST, rc);

   uniqueID = 2;
   rc = db.createCollectionSpace(&session, "bar", uniqueID, csOptions); 
   ASSERT_EQ(SDB_OK, rc);
   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(2, count);
   db.close(&session, closeDBOptions());
}


TEST_F(cs_ddl_test, test2)
{
   INT32 rc = SDB_OK;
   test_logger logger;
   outerResource resource;
   resource.logger = &logger; 
   vesselImpl db;
   test_session session;
   openDBOptions options;
   createCSOptions csOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   UINT32 createdCount = 0;
   UINT32 count = 0;

   db.initOuterResource(resource);

   rc = db.open(&session, options);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < MAX_SPACE_COUNT; ++i)
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
         ASSERT_EQ(SDB_OK, rc);
      }
   }

   if (MAX_SPACE_COUNT == createdCount)
   {
      CHAR buf[6] = {0};
      ossSnprintf(buf, 6, "%d", MAX_SPACE_COUNT);
      rc = db.createCollectionSpace(&session, buf, MAX_SPACE_COUNT, csOptions);
      ASSERT_EQ(SDB_DMS_SU_OUTRANGE, rc);
   }

   db.close(&session, closeDBOptions());

   rc = db.open(&session, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(count, createdCount);
   db.close(&session, closeDBOptions());
}


TEST_F(cs_ddl_test, test3)
{
   INT32 rc = SDB_OK;
   test_logger logger;
   outerResource resource;
   resource.logger = &logger; 
   vesselImpl db;
   test_session session;
   openDBOptions options;
   createCSOptions csOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   UINT32 count = 0;
   utilCSUniqueID uniqueID;

   db.initOuterResource(resource);
   rc = db.open(&session, options);
   ASSERT_EQ(SDB_OK, rc);

   uniqueID = 1;
   rc = db.createCollectionSpace(&session, "foo", uniqueID, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   uniqueID = 2;
   rc = db.createCollectionSpace(&session, "bar", uniqueID, csOptions);
   ASSERT_EQ(SDB_OK, rc);
   db.close(&session, closeDBOptions());

   rc = db.open(&session, options);
   ASSERT_EQ(SDB_OK, rc);
   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(2, count);
   db.close(&session, closeDBOptions());
}

TEST_F(cs_ddl_test, test4)
{
   INT32 rc = SDB_OK;
   test_logger logger;
   outerResource resource;
   resource.logger = &logger; 
   vesselImpl db;
   test_session session;
   openDBOptions options;
   createCSOptions csOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   UINT32 count = 0;
   slice content;
   const listCollectionSpaceRecord *record = NULL;
   cursorHandler c;
   utilCSUniqueID uniqueID;

   db.initOuterResource(resource);
   rc = db.open(&session, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.listCollectionSpace(&session, NULL, c);
   ASSERT_EQ(SDB_OK, rc);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_VESSEL_END_OF_CURSOR, rc);
   c.close();

   uniqueID = 1;
   rc = db.createCollectionSpace(&session, "foo1", uniqueID, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   uniqueID = 2;
   rc = db.createCollectionSpace(&session, "foo2", uniqueID, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   uniqueID = 3;
   rc = db.createCollectionSpace(&session, "foo3", uniqueID, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.listCollectionSpace(&session, NULL, c);
   ASSERT_EQ(SDB_OK, rc);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.valid());
   record = (const listCollectionSpaceRecord *)content.data();
   ASSERT_EQ(0, ossStrcmp("foo1", record->name));
   ASSERT_EQ(1, record->uniqueID);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.valid());
   record = (const listCollectionSpaceRecord *)content.data();
   ASSERT_EQ(0, ossStrcmp("foo2", record->name));
   ASSERT_EQ(2, record->uniqueID);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.valid());
   record = (const listCollectionSpaceRecord *)content.data();
   ASSERT_EQ(0, ossStrcmp("foo3", record->name));
   ASSERT_EQ(3, record->uniqueID);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_VESSEL_END_OF_CURSOR, rc);
   c.close();


   db.close(&session, closeDBOptions());

   rc = db.open(&session, options);
   ASSERT_EQ(SDB_OK, rc);
   rc = db.getCollectionSpaceCount(&session, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(3, count);

   rc = db.listCollectionSpace(&session, NULL, c);
   ASSERT_EQ(SDB_OK, rc);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.valid());
   record = (const listCollectionSpaceRecord *)content.data();
   ASSERT_EQ(0, ossStrcmp("foo1", record->name));
   ASSERT_EQ(1, record->uniqueID);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.valid());
   record = (const listCollectionSpaceRecord *)content.data();
   ASSERT_EQ(0, ossStrcmp("foo2", record->name));
   ASSERT_EQ(2, record->uniqueID);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_TRUE(content.valid());
   record = (const listCollectionSpaceRecord *)content.data();
   ASSERT_EQ(0, ossStrcmp("foo3", record->name));
   ASSERT_EQ(3, record->uniqueID);

   rc = c.getNext(&session, content);
   ASSERT_EQ(SDB_VESSEL_END_OF_CURSOR, rc);
   c.close();

   db.close(&session, closeDBOptions());
}
