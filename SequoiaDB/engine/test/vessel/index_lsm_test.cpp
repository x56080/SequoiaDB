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

   Source File Name = index_lsm_test.cpp

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
#include <gtest/gtest.h>
#include "ixm_common.hpp"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

class index_lsm_test : public testing::Test
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

// TEST_F(index_lsm_test, test1)
// {
//    INT32 rc = SDB_OK;
//    vesselImpl db;
//    outerResource resource = test_outer_resource::getResource();
//    test_executor session;
//    openDBOptions options;
//    createCLOptions clOptions;
//    options.path.dataPath = DATA_PATH;
//    options.path.indexPath = DATA_PATH;
//    options.path.lobmPath = DATA_PATH;
//    options.path.lobdPath = DATA_PATH;
//    options.path.lsmPath = LSM_PATH;

//    DATA_COLLECTION_PTR cl;
   
//    ossPoolVector<bson::BSONObj> indexes;

//    UINT32 count = 64;

//    rc = db.open(&session, &resource, options);
//    ASSERT_EQ(SDB_OK, rc);

//    rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
//    ASSERT_EQ(SDB_OK, rc);

//    rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
//    ASSERT_EQ(SDB_OK, rc);

//    rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), cl);
//    ASSERT_EQ(SDB_OK, rc);

//    for (UINT32 i = 0; i < count; ++i)
//    {
//       std::stringstream ss;
//       ss << "index" << i;
//       std::string indexName = ss.str();
//       bson::BSONObj patternObj = BSON(indexName.c_str() << 1);
//       bson::BSONObj indexDef = indexTestUtil::createIndexObj(INDEX_TYPE_LSM, 
//                                                              indexName.c_str(),
//                                                              FALSE, patternObj);
//       rc = cl->createIndex(&session, dmsBuildIndexOptions(), indexDef);
//       ASSERT_EQ(SDB_OK, rc);
//    }

//    {
//       std::stringstream ss;
//       ss << "index" << 64;
//       std::string indexName = ss.str();
//       bson::BSONObj patternObj = BSON(indexName.c_str() << 1);
//       bson::BSONObj indexDef = indexTestUtil::createIndexObj(INDEX_TYPE_LSM, 
//                                                              indexName.c_str(),
//                                                              FALSE, patternObj);
//       rc = cl->createIndex(&session, dmsBuildIndexOptions(), indexDef);
//       ASSERT_EQ(SDB_DMS_MAX_INDEX, rc);
//    }

//    rc = cl->listIndex(&session, indexes);
//    ASSERT_EQ(SDB_OK, rc);
//    ASSERT_EQ(count, indexes.size());

//    for (UINT32 i = 0; i < count; ++i)
//    {
//       const bson::BSONObj &obj = indexes.at(i);
//       ASSERT_EQ(0, ossStrcmp(IXM_LSM_TREE, obj.getStringField(IXM_TYPE_FIELD)));
//       std::stringstream ss;
//       ss << "index" << i;
//       ASSERT_EQ(0, ss.str().compare(obj.getStringField(IXM_NAME_FIELD)));
//       ASSERT_EQ(INDEX_STATUS_NORMAL, obj.getIntField(IXM_STATUS_FIELD));
//    }

//    cl->close();
//    db.close(&session, closeDBOptions());

//    rc = db.open(&session, &resource, options);
//    ASSERT_EQ(SDB_OK, rc);

//    rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), cl);
//    ASSERT_EQ(SDB_OK, rc);

//    indexes.clear();
//    rc = cl->listIndex(&session, indexes);
//    ASSERT_EQ(SDB_OK, rc);
//    ASSERT_EQ(count, indexes.size());

//    for (UINT32 i = 0; i < count; ++i)
//    {
//       const bson::BSONObj &obj = indexes.at(i);
//       ASSERT_EQ(0, ossStrcmp(IXM_LSM_TREE, obj.getStringField(IXM_TYPE_FIELD)));
//       std::stringstream ss;
//       ss << "index" << i;
//       ASSERT_EQ(0, ss.str().compare(obj.getStringField(IXM_NAME_FIELD)));
//       ASSERT_EQ(INDEX_STATUS_NORMAL, obj.getIntField(IXM_STATUS_FIELD));
//    }

//    cl->close();
//    db.close(&session, closeDBOptions());
// }

// TEST_F(index_lsm_test, test2)
// {
//    INT32 rc = SDB_OK;
//    vesselImpl db;
//    outerResource resource = test_outer_resource::getResource();
//    test_executor session;
//    openDBOptions options;
//    createCLOptions clOptions;
//    options.path.dataPath = DATA_PATH;
//    options.path.indexPath = DATA_PATH;
//    options.path.lobmPath = DATA_PATH;
//    options.path.lobdPath = DATA_PATH;
//    options.path.lsmPath = LSM_PATH;

//    DATA_COLLECTION_PTR cl;

//    static const UINT32 pad_size = 1024;
//    CHAR pad[pad_size] = {};
   
//    ossPoolVector<bson::BSONObj> indexes;
//    bson::BSONObjBuilder builder;
//    collectionSpaceId id;

//    UINT32 count = 8;

//    rc = db.open(&session, &resource, options);
//    ASSERT_EQ(SDB_OK, rc);

//    rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
//    ASSERT_EQ(SDB_OK, rc);

//    rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
//    ASSERT_EQ(SDB_OK, rc);

//    rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), cl);
//    ASSERT_EQ(SDB_OK, rc);

//    for (UINT32 i = 0; i < 200000; ++i)
//    {
//       utilInsertResult  r;
//       builder.reset();
//       builder.append("a", i);
//       builder.appendBinData("b", pad_size, bson::BinDataType::bdtCustom, pad);
//       bson::BSONObj obj = builder.done();
//       rc = cl->insertRecord(&session, obj, dmsInsertRecordOptions(), &r);
//       ASSERT_EQ(rc, SDB_OK);
//    }

//    for (UINT32 i = 0; i < count; ++i)
//    {
//       std::stringstream ss;
//       ss << "index" << i;
//       std::string indexName = ss.str();
//       bson::BSONObj patternObj = BSON(indexName.c_str() << 1);
//       bson::BSONObj indexDef = indexTestUtil::createIndexObj(INDEX_TYPE_LSM, 
//                                                              indexName.c_str(),
//                                                              FALSE, patternObj);
//       rc = cl->createIndex(&session, dmsBuildIndexOptions(), indexDef);
//       ASSERT_EQ(SDB_OK, rc);
//    }

//    indexes.clear();
//    rc = cl->listIndex(&session, indexes);
//    ASSERT_EQ(SDB_OK, rc);
//    ASSERT_EQ(count, indexes.size());

//    for (UINT32 i = 0; i < count; ++i)
//    {
//       std::stringstream ss;
//       ss << "index" << i;
//       const bson::BSONObj &obj = indexes.at(i);
//       ASSERT_EQ(0, ossStrcmp(IXM_LSM_TREE, obj.getStringField(IXM_TYPE_FIELD)));
//       std::string name = ss.str();
//       ASSERT_EQ(0, name.compare(obj.getStringField(IXM_NAME_FIELD)));
//       ASSERT_EQ(INDEX_STATUS_NORMAL, obj.getIntField(IXM_STATUS_FIELD));
//    }

//    cl->close();
//    db.close(&session, closeDBOptions());

//    rc = db.open(&session, &resource, options);
//    ASSERT_EQ(SDB_OK, rc);

//    rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), cl);
//    ASSERT_EQ(SDB_OK, rc);

//    indexes.clear();
//    rc = cl->listIndex(&session, indexes);
//    ASSERT_EQ(SDB_OK, rc);
//    ASSERT_EQ(count, indexes.size());

//    for (UINT32 i = 0; i < count; ++i)
//    {
//       std::stringstream ss;
//       ss << "index" << i;
//       const bson::BSONObj &obj = indexes.at(i);
//       ASSERT_EQ(0, ossStrcmp(IXM_LSM_TREE, obj.getStringField(IXM_TYPE_FIELD)));
//       std::string name = ss.str();
//       ASSERT_EQ(0, name.compare(obj.getStringField(IXM_NAME_FIELD)));
//       ASSERT_EQ(INDEX_STATUS_NORMAL, obj.getIntField(IXM_STATUS_FIELD));
//    }

//    cl->close();
//    db.close(&session, closeDBOptions());
// }

// /// unique index
// TEST_F(index_lsm_test, test3)
// {
//    INT32 rc = SDB_OK;
//    vesselImpl db;
//    outerResource resource = test_outer_resource::getResource();
//    test_executor session;
//    openDBOptions options;
//    createCLOptions clOptions;
//    options.path.dataPath = DATA_PATH;
//    options.path.indexPath = DATA_PATH;
//    options.path.lobmPath = DATA_PATH;
//    options.path.lobdPath = DATA_PATH;
//    options.path.lsmPath = LSM_PATH;

//    DATA_COLLECTION_PTR cl;
   
//    ossPoolVector<bson::BSONObj> indexes;
//    collectionSpaceId id;

//    rc = db.open(&session, &resource, options);
//    ASSERT_EQ(SDB_OK, rc);

//    rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
//    ASSERT_EQ(SDB_OK, rc);

//    rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
//    ASSERT_EQ(SDB_OK, rc);

//    rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), cl);
//    ASSERT_EQ(SDB_OK, rc);

//    std::string indexName("a");
//    bson::BSONObj patternObj = BSON(indexName.c_str() << 1);
//       bson::BSONObj indexDef = indexTestUtil::createIndexObj(INDEX_TYPE_LSM, 
//                                                              indexName.c_str(),
//                                                              TRUE, patternObj);
//    rc = cl->createIndex(&session, dmsBuildIndexOptions(), indexDef);
//    ASSERT_EQ(SDB_OK, rc);

//    bson::BSONObjBuilder builder;
//    for (UINT32 i = 0; i < 1000; ++i)
//    {
//       utilInsertResult res;
//       builder.reset();
//       builder.append("a", i);
//       bson::BSONObj obj = builder.done();
//       rc = cl->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
//       ASSERT_EQ(SDB_OK, rc);
//    }

//    for (UINT32 i = 0; i < 1000; ++i)
//    {
//       utilInsertResult res;
//       builder.reset();
//       builder.append("a", i);
//       bson::BSONObj obj = builder.done();
//       rc = cl->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
//       ASSERT_EQ(SDB_IXM_DUP_KEY, rc);
//    }

//    cl->close();
//    db.close(&session, closeDBOptions());
// }

// void duplicated_insert(vesselImpl *db,
//                        const CHAR *fullName,
//                        UINT32 count, UINT32 range)
// {
//    test_executor session;
//    bson::BSONObjBuilder builder;
//    DATA_COLLECTION_PTR handler;
//    INT32 rc = db->openCL(&session, fullName, dmsOpenCLOptions(), handler);
//    ASSERT_EQ(SDB_OK, rc);

//    for (UINT32 i = 0; i < count; ++i)
//    {
//       UINT32 r = ossRand() % range;
//       builder.reset();
//       builder.append("a", r);
//       bson::BSONObj obj = builder.done();
//       utilInsertResult res;
//       rc = handler->insertRecord(&session, obj, dmsInsertRecordOptions(), &res);
//       ASSERT_TRUE((SDB_OK == rc || SDB_IXM_DUP_KEY == rc));
//    }
//    handler->close();
// }

// /// unique index
// TEST_F(index_lsm_test, test4)
// {
//    INT32 rc = SDB_OK;
//    vesselImpl db;
//    outerResource resource = test_outer_resource::getResource();
//    test_executor session;
//    openDBOptions options;
//    createCLOptions clOptions;
//    options.path.dataPath = DATA_PATH;
//    options.path.indexPath = DATA_PATH;
//    options.path.lobmPath = DATA_PATH;
//    options.path.lobdPath = DATA_PATH;
//    options.path.lsmPath = LSM_PATH;

//    DATA_COLLECTION_PTR cl;

//    static const UINT32 threadCount = 6;
//    std::thread threads[threadCount];
//    UINT32 range = 100;

//    rc = db.open(&session, &resource, options);
//    ASSERT_EQ(SDB_OK, rc);

//    rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
//    ASSERT_EQ(SDB_OK, rc);

//    rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
//    ASSERT_EQ(SDB_OK, rc);

//    rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), cl);
//    ASSERT_EQ(SDB_OK, rc);

//    std::string indexName("a");
//    bson::BSONObj patternObj = BSON(indexName.c_str() << 1);
//       bson::BSONObj indexDef = indexTestUtil::createIndexObj(INDEX_TYPE_LSM, 
//                                                              indexName.c_str(),
//                                                              TRUE, patternObj);
//    rc = cl->createIndex(&session, dmsBuildIndexOptions(), indexDef);
//    ASSERT_EQ(SDB_OK, rc);

//    for (UINT32 i = 0; i < threadCount; ++i)
//    {
//       threads[i] = std::move(std::thread(duplicated_insert, &db,
//                                          "foo.bar", 100000, range));
//    }

//    for (UINT32 i = 0; i < threadCount; ++i)
//    {
//       threads[i].join();
//    }

//    UINT64 recordCount = 0;
//    rc = cl->getRecordCount(&session, recordCount);
//    ASSERT_EQ(SDB_OK, rc);
//    ASSERT_GE(range, recordCount);
//    ASSERT_GT(recordCount, 0);

//    cl->close();
//    db.close(&session, closeDBOptions());
// }