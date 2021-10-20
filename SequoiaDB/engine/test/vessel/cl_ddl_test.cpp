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

#include <thread> // c++11


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
   vesselImpl db;
   outerResource resource;
   
   resource.logger = test_logger::instance();
   resource.sessionMgr = test_session_mgr::instance(); 
   test_session session(test_logger::instance());
   openDBOptions options;
   createCSOptions csOptions;
   createCLOptions clOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   cursorHandler cursor;
   slice slice;
   bson::BSONObj record;

   rc = db.open(&session, &resource, options);
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

   record = bson::BSONObj(slice.getRPtr());
   ASSERT_EQ(0, record.getIntField(CL_DUMP_RECORD_FIELD_MB_ID));
   ASSERT_EQ(1, record.getIntField(CL_DUMP_RECORD_FIELD_INNER_ID));
   ASSERT_EQ(0, record.getIntField(CL_DUMP_RECORD_FIELD_CL_LOGICAL_ID));
   ASSERT_EQ(0, ossStrcmp("bar1", record.getStringField(CL_DUMP_RECORD_FIELD_NAME)));

   rc = cursor.getNext(&session, slice);
   record = bson::BSONObj(slice.getRPtr());
   ASSERT_EQ(1, record.getIntField(CL_DUMP_RECORD_FIELD_MB_ID));
   ASSERT_EQ(2, record.getIntField(CL_DUMP_RECORD_FIELD_INNER_ID));
   ASSERT_EQ(1, record.getIntField(CL_DUMP_RECORD_FIELD_CL_LOGICAL_ID));
   ASSERT_EQ(0, ossStrcmp("bar2", record.getStringField(CL_DUMP_RECORD_FIELD_NAME)));

   rc = cursor.getNext(&session, slice);
   ASSERT_EQ(SDB_VESSEL_EOC, rc);

   db.close(&session, closeDBOptions());
}

TEST_F(cl_ddl_test, test2)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource;
   resource.logger = test_logger::instance();
   resource.sessionMgr = test_session_mgr::instance(); 
   test_session session(test_logger::instance());
   openDBOptions options;
   createCSOptions csOptions;
   createCLOptions clOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar1", 1, clOptions);
   ASSERT_EQ(SDB_OK, rc);
   rc = db.createCollection(&session, "foo", "bar2", 2, clOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar1", 3, clOptions);
   ASSERT_EQ(SDB_DMS_EXIST, rc);
   rc = db.createCollection(&session, "foo", "bar2", 3, clOptions);
   ASSERT_EQ(SDB_DMS_EXIST, rc);
   rc = db.createCollection(&session, "foo", "bar3", 3, clOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

    rc = db.open(&session, &resource, options);
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

   vesselImpl db;
   outerResource resource;
   resource.logger = test_logger::instance();
   resource.sessionMgr = test_session_mgr::instance(); 
   test_session session(test_logger::instance());
   openDBOptions options;
   createCSOptions csOptions;
   createCLOptions clOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   UINT32 creatingCount = 65535;
   UINT32 count = 0;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < creatingCount; ++i)
   {
      CHAR name[32] = {0};
      sprintf(name, "%s%d", "bar", i);
      rc = db.createCollection(&session, "foo", name, i + 1, clOptions);
      ASSERT_EQ(SDB_OK, rc);
   }

   rc = db.createCollection(&session, "foo", "bar65535", 65536, clOptions);
   ASSERT_EQ(SDB_VESSEL_OUT_OF_MBID_RESOURCE, rc);

   rc = db.getCollectionCount(&session, "foo", count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(creatingCount, count);
   
   
   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);


   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);
   rc = db.getCollectionCount(&session, "foo", count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(creatingCount, count);
   
   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

}

void thread_create_cl(vesselImpl *db, engine::vessel::ISession *session, UINT32 innerId, UINT32 count)
{
   INT32 rc = SDB_OK;
   createCLOptions clOptions;
   for (UINT32 i = 0; i < count; ++i)
   {
      CHAR name[32] = {0};
      sprintf(name, "%s%d", "bar", innerId + i);
      rc = db->createCollection(session, "foo", name, innerId + i, clOptions);
      ASSERT_EQ(SDB_OK, rc);
   }
}

TEST_F(cl_ddl_test, test4)
{
   INT32 rc = SDB_OK;

   vesselImpl db;
   outerResource resource;
   resource.logger = test_logger::instance();
   resource.sessionMgr = test_session_mgr::instance(); 
   test_session session(test_logger::instance());
   openDBOptions options;
   createCSOptions csOptions;
   createCLOptions clOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   UINT32 creatingCount = 65535;
   UINT32 count = 0;
   static const UINT32 threadCount = 4;
   std::thread threads[threadCount];
   UINT32 innerID = 1;
   UINT32 countPerThread = creatingCount / threadCount;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(thread_create_cl,
                                         &db, &session, innerID, countPerThread));
      innerID += countPerThread;
   }

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i].join();
   }

   rc = db.getCollectionCount(&session, "foo", count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(countPerThread * threadCount, count);
   
   
   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);


   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);
   rc = db.getCollectionCount(&session, "foo", count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(countPerThread * threadCount, count);
   
   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

}