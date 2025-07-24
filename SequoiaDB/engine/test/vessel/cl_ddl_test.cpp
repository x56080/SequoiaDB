/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = cl_ddl_test.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "test_def.h"
#include "vessel/vesselImpl.h"
#include "vessel/requestContext.h"
#include <gtest/gtest.h>
#include "ossUtil.hpp"
#include "dpsLogRecord.hpp"
#include "vessel/listCollectionSpaceDef.h"
#include "vessel/listCollectionsDef.h"
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
   outerResource resource = test_outer_resource::getResource();

   test_executor executor;
   openDBOptions options;

   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobmPath = DATA_PATH;
   options.path.lobdPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   
   bson::BSONObj adjunct;
   dmsCreateCSOptions csOptions;
   dmsCreateCLOptions clOptions;
   DATA_CURSOR_PTR cursor;
   bson::BSONObj record;

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&executor, "foo", 1, csOptions, adjunct);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&executor, "foo.bar1", 1, clOptions, adjunct);
   ASSERT_EQ(SDB_OK, rc);
   rc = db.createCL(&executor, "foo.bar1", 2, clOptions, adjunct);
   ASSERT_EQ(SDB_DMS_EXIST, rc);
   rc = db.createCL(&executor, "foo.bar2", 1, clOptions, adjunct);
   ASSERT_EQ(SDB_DMS_EXIST, rc);
   rc = db.createCL(&executor, "foo.bar2", 2, clOptions, adjunct);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.listCL(&executor, "foo", cursor);
   ASSERT_EQ(SDB_OK, rc);

   rc = cursor->fetchNext(&executor);
   ASSERT_EQ(SDB_OK, rc);
   record = cursor->getBsonRecord();
   ASSERT_EQ(0, record.getIntField(CL_DUMP_RECORD_FIELD_MB_ID));
   ASSERT_EQ(1, record.getIntField(CL_DUMP_RECORD_FIELD_INNER_ID));
   ASSERT_EQ(0, record.getIntField(CL_DUMP_RECORD_FIELD_CL_LOGICAL_ID));
   ASSERT_EQ(0, ossStrcmp("bar1", record.getStringField(CL_DUMP_RECORD_FIELD_NAME)));

   rc = cursor->fetchNext(&executor);
   ASSERT_EQ(SDB_OK, rc);
   record = cursor->getBsonRecord();
   ASSERT_EQ(1, record.getIntField(CL_DUMP_RECORD_FIELD_MB_ID));
   ASSERT_EQ(2, record.getIntField(CL_DUMP_RECORD_FIELD_INNER_ID));
   ASSERT_EQ(1, record.getIntField(CL_DUMP_RECORD_FIELD_CL_LOGICAL_ID));
   ASSERT_EQ(0, ossStrcmp("bar2", record.getStringField(CL_DUMP_RECORD_FIELD_NAME)));

   rc = cursor->fetchNext(&executor);
   ASSERT_EQ(SDB_DMS_EOC, rc);

   db.close(&executor, closeDBOptions());
}

TEST_F(cl_ddl_test, test2)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobmPath = DATA_PATH;
   options.path.lobdPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar1", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);
   rc = db.createCL(&session, "foo.bar2", 2, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar1", 3, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_DMS_EXIST, rc);
   rc = db.createCL(&session, "foo.bar2", 3, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_DMS_EXIST, rc);
   rc = db.createCL(&session, "foo.bar3", 3, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

    rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar1", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_DMS_EXIST, rc);
   rc = db.createCL(&session, "foo.bar2", 2, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_DMS_EXIST, rc);
   rc = db.createCL(&session, "foo.bar3", 3, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_DMS_EXIST, rc);
   rc = db.createCL(&session, "foo.bar4", 4, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

TEST_F(cl_ddl_test, test3)
{
   INT32 rc = SDB_OK;

   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobmPath = DATA_PATH;
   options.path.lobdPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   UINT32 creatingCount = 65535;
   UINT32 count = 0;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < creatingCount; ++i)
   {
      CHAR name[32] = {0};
      sprintf(name, "%s%d", "foo.bar", i);
      rc = db.createCL(&session, name, i + 1, dmsCreateCLOptions(), bson::BSONObj());
      ASSERT_EQ(SDB_OK, rc);
   }

   rc = db.createCL(&session, "foo.bar65535", 65536, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_VESSEL_OUT_OF_MBID_RESOURCE, rc);

   rc = db.getCLCount(&session, "foo", count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(creatingCount, count);
   
   
   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);


   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);
   rc = db.getCLCount(&session, "foo", count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(creatingCount, count);
   
   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

}

void thread_create_cl(vesselImpl *db, engine::IExecutor *session, UINT32 innerId, UINT32 count)
{
   INT32 rc = SDB_OK;
   createCLOptions clOptions;
   for (UINT32 i = 0; i < count; ++i)
   {
      CHAR name[32] = {0};
      sprintf(name, "%s%d", "foo.bar", innerId + i);
      rc = db->createCL(session, name, innerId + i, dmsCreateCLOptions(), bson::BSONObj());
      ASSERT_EQ(SDB_OK, rc);
   }
}

TEST_F(cl_ddl_test, test4)
{
   INT32 rc = SDB_OK;

   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   dmsCreateCSOptions csOptions;
   dmsCreateCLOptions clOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobmPath = DATA_PATH;
   options.path.lobdPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   UINT32 creatingCount = 65535;
   UINT32 count = 0;
   static const UINT32 threadCount = 4;
   std::thread threads[threadCount];
   UINT32 innerID = 1;
   UINT32 countPerThread = creatingCount / threadCount;
   collectionSpaceId identifier;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, csOptions, bson::BSONObj());
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

   rc = db.getCLCount(&session, "foo", count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(countPerThread * threadCount, count);
   
   
   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);


   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);
   rc = db.getCLCount(&session, "foo", count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(countPerThread * threadCount, count);
   
   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

}

TEST_F(cl_ddl_test, base_remove_cl_1)
{
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();

   test_executor executor;
   openDBOptions options;

   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobmPath = DATA_PATH;
   options.path.lobdPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   INT32 rc = SDB_OK;
   UINT32 count = 0;

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&executor, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&executor, "foo.bar1", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);
   rc = db.createCL(&executor, "foo.bar2", 2, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);
   rc = db.createCL(&executor, "foo.bar3", 3, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.getCLCount(&executor, "foo", count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(3, count);

   rc = db.removeCL(&executor, "foo.bar1", dmsRemoveCLOptions());
   ASSERT_EQ(SDB_OK, rc);
   rc = db.getCLCount(&executor, "foo", count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(2, count);
   rc = db.removeCL(&executor, "foo.bar1", dmsRemoveCLOptions());
   ASSERT_EQ(SDB_DMS_NOTEXIST, rc);

   rc = db.removeCL(&executor, "foo.bar2", dmsRemoveCLOptions());
   ASSERT_EQ(SDB_OK, rc);
   rc = db.getCLCount(&executor, "foo", count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(1, count);
   rc = db.removeCL(&executor, "foo.bar2", dmsRemoveCLOptions());
   ASSERT_EQ(SDB_DMS_NOTEXIST, rc);

   rc = db.removeCL(&executor, "foo.bar3", dmsRemoveCLOptions());
   ASSERT_EQ(SDB_OK, rc);
   rc = db.getCLCount(&executor, "foo", count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, count);
   rc = db.removeCL(&executor, "foo.bar3", dmsRemoveCLOptions());
   ASSERT_EQ(SDB_DMS_NOTEXIST, rc);

   rc = db.close(&executor, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);
   rc = db.getCLCount(&executor, "foo", count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, count);

   rc = db.close(&executor, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

TEST_F(cl_ddl_test, base_remove_cl_2)
{
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();

   test_executor executor;
   openDBOptions options;

   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobmPath = DATA_PATH;
   options.path.lobdPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   INT32 rc = SDB_OK;

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&executor, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&executor, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   DATA_COLLECTION_PTR cl;
   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), cl);
   ASSERT_EQ(SDB_OK, rc);

   bson::BSONObj indexDef;
   indexDef = indexTestUtil::createIndexObj(INDEX_TYPE_HYBRID_TREE,
                                            "index1", FALSE,
                                            BSON("a" << 1));
   rc = cl->createIndex(&executor, dmsBuildIndexOptions(), indexDef);
   ASSERT_EQ(SDB_OK, rc);

   indexDef = indexTestUtil::createIndexObj(INDEX_TYPE_HYBRID_TREE,
                                            "index2", FALSE,
                                            BSON("b" << 1));
   rc = cl->createIndex(&executor, dmsBuildIndexOptions(), indexDef);
   ASSERT_EQ(SDB_OK, rc);

   UINT32 insertCount = 10000;
   bson::BSONObjBuilder builder;
   constexpr UINT32 _PAD_SIZE = 1024;
   CHAR pad[_PAD_SIZE] = {};
   ossMemset(pad, 'a', sizeof(pad) - 1);

   for (UINT32 i = 0; i < insertCount; ++i)
   {
      builder.append("a", i);
      builder.append("b", i);
      builder.append("c", pad);
      rc = cl->insertRecord(&executor, builder.done(), dmsInsertRecordOptions(), NULL);
      ASSERT_EQ(SDB_OK, rc);
      builder.reset();
   }

   rc = db.removeCL(&executor, "foo.bar", dmsRemoveCLOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.removeCL(&executor, "foo.bar", dmsRemoveCLOptions());
   ASSERT_EQ(SDB_DMS_NOTEXIST, rc);

   UINT64 count = 0;
   rc = cl->getRecordCount(&executor, count);
   ASSERT_EQ(SDB_DMS_NOTEXIST, rc);

   rc = db.close(&executor, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&executor, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.close(&executor, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

TEST_F(cl_ddl_test, base_truncate_cl_1)
{
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();

   test_executor executor;
   openDBOptions options;

   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobmPath = DATA_PATH;
   options.path.lobdPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   INT32 rc = SDB_OK;

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&executor, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&executor, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   DATA_COLLECTION_PTR cl;
   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), cl);
   ASSERT_EQ(SDB_OK, rc);

   UINT32 insertCount = 10000;
   bson::BSONObjBuilder builder;
   constexpr UINT32 _PAD_SIZE = 1024;
   CHAR pad[_PAD_SIZE] = {};
   ossMemset(pad, 'a', sizeof(pad) - 1);
   builder.append("a", pad);
   bson::BSONObj obj = builder.done();
   for (UINT32 i = 0; i < insertCount; ++i)
   {
      rc = cl->insertRecord(&executor, obj, dmsInsertRecordOptions(), NULL);
      ASSERT_EQ(SDB_OK, rc);
   }

   UINT64 count = 0;
   rc = cl->getRecordCount(&executor, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(insertCount, count);

   rc = cl->truncate(&executor, dmsTruncateCLOptions());
   ASSERT_EQ(SDB_OK, rc);

   count = 0;
   rc = cl->getRecordCount(&executor, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, count);

   rc = db.close(&executor, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   cl.reset();
   rc = db.openCL(&executor, "foo.bar", dmsOpenCLOptions(), cl);
   ASSERT_EQ(SDB_OK, rc);

   rc = cl->getRecordCount(&executor, count);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, count);

   rc = db.close(&executor, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

/*
Name: base_idx_ds_file_creation_timing
Description: 
   集合空间的idx.ds文件创建时机
   1. 创建单个cs,cs下创建单个cl
   2. 验证idx.ds文件是否被创建
Expected Result: 
   idx.ds文件未被创建
*/
TEST_F(cl_ddl_test, base_idx_ds_file_creation_timing)
{
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();

   test_executor executor;
   openDBOptions options;

   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobmPath = DATA_PATH;
   options.path.lobdPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   INT32 rc = SDB_OK;

   rc = db.open(&executor, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&executor, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&executor, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   fs::path DATA_FS_PATH(DATA_PATH);
   const CHAR* csDir = "_cs_00000";
   const CHAR* idxDsFileName = "idx.ds.000000";
   fs::path IDX_DS_FS_PATH = DATA_FS_PATH/csDir/idxDsFileName;
   BOOLEAN isExist = fs::exists(IDX_DS_FS_PATH);
   ASSERT_EQ(FALSE, isExist);

   rc = db.close(&executor, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(FALSE, isExist);
}