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

   Source File Name = cl_ddl_test.cpp

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


class cl_ddl_test : public testing::Test
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

TEST_F(cl_ddl_test, test1)
{
   INT32 rc = SDB_OK;
   test_logger logger;
   vesselImpl db;
   outerResource resource;
   resource.logger = &logger; 
   test_session session;
   openDBOptions options;
   createCSOptions csOptions;
   createCLOptions clOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   cursorHandler cursor;
   slice slice;
   const listCollectionsRecord *record = NULL;

   db.initOuterResource(resource);
   rc = db.open(&session, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar1", 1, clOptions);
   ASSERT_EQ(SDB_OK, rc);
   rc = db.createCollection(&session, "foo", "bar1", 2, clOptions);
   ASSERT_EQ(SDB_DMS_EXIST, rc);
   rc = db.createCollection(&session, "foo", "bar2", 1, clOptions);
   ASSERT_EQ(SDB_DMS_EXIST, rc);
   rc = db.createCollection(&session, "foo", "bar2", 2, clOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.listCollections(&session, "foo", NULL, cursor);
   ASSERT_EQ(SDB_OK, rc);

   rc = cursor.getNext(&session, slice);
   ASSERT_EQ(SDB_OK, rc);
   record = (const listCollectionsRecord *)(slice.data());
   ASSERT_EQ(0, record->mbID);
   ASSERT_EQ(1, record->csUniqueID);
   ASSERT_EQ(1, record->clInnerID);
   ASSERT_EQ(0, ossStrcmp("bar1", record->name));

   rc = cursor.getNext(&session, slice);
   ASSERT_EQ(SDB_OK, rc);
   record = (const listCollectionsRecord *)(slice.data());
   ASSERT_EQ(1, record->mbID);
   ASSERT_EQ(1, record->csUniqueID);
   ASSERT_EQ(2, record->clInnerID);
   ASSERT_EQ(0, ossStrcmp("bar2", record->name));

   rc = cursor.getNext(&session, slice);
   ASSERT_EQ(SDB_VESSEL_END_OF_CURSOR, rc);

   db.close(&session, closeDBOptions());
}

TEST_F(cl_ddl_test, test2)
{
   INT32 rc = SDB_OK;
   test_logger logger;
   vesselImpl db;
   outerResource resource;
   resource.logger = &logger; 
   test_session session;
   openDBOptions options;
   createCSOptions csOptions;
   createCLOptions clOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;

   db.initOuterResource(resource);
   rc = db.open(&session, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar1", 1, clOptions);
   ASSERT_EQ(SDB_OK, rc);
   rc = db.createCollection(&session, "foo", "bar2", 2, clOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&session, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar1", 3, clOptions);
   ASSERT_EQ(SDB_DMS_EXIST, rc);
   rc = db.createCollection(&session, "foo", "bar2", 3, clOptions);
   ASSERT_EQ(SDB_DMS_EXIST, rc);
   rc = db.createCollection(&session, "foo", "bar3", 3, clOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

    rc = db.open(&session, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar1", 1, clOptions);
   ASSERT_EQ(SDB_DMS_EXIST, rc);
   rc = db.createCollection(&session, "foo", "bar2", 2, clOptions);
   ASSERT_EQ(SDB_DMS_EXIST, rc);
   rc = db.createCollection(&session, "foo", "bar3", 3, clOptions);
   ASSERT_EQ(SDB_DMS_EXIST, rc);
   rc = db.createCollection(&session, "foo", "bar4", 4, clOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

TEST_F(cl_ddl_test, test3)
{
   INT32 rc = SDB_OK;
   test_logger logger;
   vesselImpl db;
   outerResource resource;
   resource.logger = &logger; 
   test_session session;
   openDBOptions options;
   options.extendFileWithSparse = TRUE;
   createCSOptions csOptions;
   createCLOptions clOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   UINT32 creatingCount = 65535;
   UINT32 count = 0;

   db.initOuterResource(resource);
   rc = db.open(&session, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < 65535; ++i)
   {
      CHAR name[32] = {0};
      sprintf(name, "%s%d", "bar", i);
      rc = db.createCollection(&session, "foo", name, i + 1, clOptions);
      ASSERT_EQ(SDB_OK, rc);
   }

   rc = db.createCollection(&session, "foo", "bar65535", 65536, clOptions);
   ASSERT_EQ(SDB_DMS_NOSPC, rc);

   rc = db.getCollectionCount(&session, "foo", count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(creatingCount, count);
   
   
   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);


   rc = db.open(&session, options);
   ASSERT_EQ(SDB_OK, rc);
   rc = db.getCollectionCount(&session, "foo", count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(creatingCount, count);
   
   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
   

}