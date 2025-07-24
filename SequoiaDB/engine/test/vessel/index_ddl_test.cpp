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

   Source File Name = index_ddl_test.cpp

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
#include "../bson/bson.hpp"
#include "pd.hpp"
#include "vessel/collectionOptions.h"
#include "mthMatchTree.hpp"
#include <iostream>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

class index_ddl_test : public testing::Test
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

ossPoolString combine(const CHAR *prefix, UINT32 i)
{
   SDB_ASSERT(NULL != prefix, "can not be null");
   CHAR buf[32] = {};
   ossItoa(i, buf, 32);
   ossPoolString str;
   str.append(prefix).append(buf);
   return std::move(str);
}

/*
Name: base_remove_1
Description: 
   索引的重复创建与删除
   1. 创建单个cs,cs下创建单个cl
   2. cl创建多个索引
   3. cl删除多个索引
   4. cl重新创建多个相同索引
   5. cl删除多个索引
Expected Result: 
   在创建和删除索引过程中进行验证,索引数量符合预期
*/
void remove_index1(INDEX_TYPE type)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;

   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   DATA_COLLECTION_PTR handler;

   constexpr INT32 indexCount = 32;

   ossPoolVector<bson::BSONObj> indexes;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (INT32 i = 0; i < indexCount; ++i)
   {
      ossPoolString indexName = combine("index", i);
      bson::BSONObj pattern = BSON(indexName.c_str() << 1);
      bson::BSONObj indexDef = indexTestUtil::createIndexObj(type, indexName.c_str(),
                                                             FALSE, pattern);
      rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
      ASSERT_EQ(SDB_OK, rc);
   }

   rc = handler->listIndex(&session, indexes);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(indexCount, indexes.size());

   for (INT32 i = 0; i < indexCount; ++i)
   {
      ossPoolString indexName = combine("index", i);
      bson::BSONObj pattern = BSON(indexName.c_str() << 1);
      bson::BSONObj indexDef = indexTestUtil::createIndexObj(type, indexName.c_str(),
                                                             FALSE, pattern);
      rc = handler->removeIndex(&session, indexName.c_str());
      ASSERT_EQ(SDB_OK, rc);
   }

   indexes.clear();
   rc = handler->listIndex(&session, indexes);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, indexes.size());

   for (INT32 i = 0; i < indexCount; ++i)
   {
      ossPoolString indexName = combine("index", i);
      bson::BSONObj pattern = BSON(indexName.c_str() << 1);
      bson::BSONObj indexDef = indexTestUtil::createIndexObj(type, indexName.c_str(),
                                                             FALSE, pattern);
      rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
      ASSERT_EQ(SDB_OK, rc);
   }

   indexes.clear();
   rc = handler->listIndex(&session, indexes);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(indexCount, indexes.size());

   for (INT32 i = 0; i < indexCount; ++i)
   {
      ossPoolString indexName = combine("index", i);
      bson::BSONObj pattern = BSON(indexName.c_str() << 1);
      bson::BSONObj indexDef = indexTestUtil::createIndexObj(type, indexName.c_str(),
                                                             FALSE, pattern);
      rc = handler->removeIndex(&session, indexName.c_str());
      ASSERT_EQ(SDB_OK, rc);
   }

   indexes.clear();
   rc = handler->listIndex(&session, indexes);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, indexes.size());

   handler->close();
   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

TEST_F(index_ddl_test, base_remove_1_1)
{
   remove_index1(INDEX_TYPE_HYBRID_TREE);
}


/*
Name: base_remove_2
Description: 
   索引扫描结果验证
   1. 创建单个cs,cs下创建单个cl
   2. cl创建单个索引，并插入数据
   3. 删除该索引
   4. 进行索引扫描
   5. 验证扫描结果
Expected Result: 
   索引扫描失败，索引不存在
*/
void remove_index2(INDEX_TYPE type)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;

   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   DATA_COLLECTION_PTR handler;

   constexpr const CHAR *indexName = "index";
   constexpr const CHAR *fieldName = "a";
   constexpr UINT32 recordCount = 100000;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   {
      bson::BSONObj pattern = BSON(fieldName << 1);
      bson::BSONObj indexDef = indexTestUtil::createIndexObj(type, indexName,
                                                             FALSE, pattern);
      rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
      ASSERT_EQ(SDB_OK, rc);
   }

   bson::BSONObjBuilder builder;
   CHAR pad[128];
   ossMemset(pad, 'a', sizeof(pad));
   std::string key;
   key.assign(pad);
   for (UINT32 i = 0; i < recordCount; ++i)
   {
      key.resize(sizeof(pad));
      CHAR buf[16];
      ossItoa(i, buf, sizeof(buf));
      key.append(buf);
      builder.append(fieldName, key.c_str());
      rc = handler->insertRecord(&session, builder.done(), dmsInsertRecordOptions(), NULL);
      ASSERT_EQ(SDB_OK, rc);
      builder.reset();
   }

   handler->removeIndex(&session, indexName);

   {
      bson::BSONObj match = BSON(fieldName << BSON("$gte" << 0));
      mthMatchTree mt;
      rc = mt.loadPattern(match, FALSE);
      ASSERT_EQ(SDB_OK, rc);
      rtnPredicateSet ps;
      rc = mt.calcPredicate(ps, NULL);
      ASSERT_EQ(SDB_OK, rc);
      rtnPredicateList predicates;
      UINT32 lvl = 0;
      bson::BSONObj pattern = BSON(fieldName << 1);
      rc = predicates.initialize(ps, pattern, 1, lvl);
      ASSERT_EQ(SDB_OK, rc);
      DATA_CURSOR_PTR cursor;
      rc = handler->scanIndex(&session, indexName, predicates,
                              dmsIndexScanOptions(), cursor);
      ASSERT_EQ(SDB_IXM_NOTEXIST, rc);
   }

   handler->close();
   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

TEST_F(index_ddl_test, base_remove_2_1)
{
   remove_index2(INDEX_TYPE_HYBRID_TREE);
}

/*
Name: base_remove_index_test3
Description: 
   索引重启删除测试
   1. 创建单个cs,cs下创建单个cl
   2. cl创建多个索引,并删除其中一个，验证索引数量
   3. 重启vessel
   4. cl删除剩余索引,验证索引数量
   5. 再次重启vessel
   6. 验证索引数量
Expected Result: 
   索引数量验证正确
*/
void remove_index_test3(INDEX_TYPE type)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   DATA_COLLECTION_PTR handler;
   ossPoolVector<bson::BSONObj> indexes;

   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   {
      string indexName = "index1";
      bson::BSONObj pattern = BSON(indexName.c_str() << 1);
      bson::BSONObj indexDef = indexTestUtil::createIndexObj(type, indexName.c_str(),
                                                             FALSE, pattern);
      rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
      ASSERT_EQ(SDB_OK, rc);
   }
   {
      string indexName = "index2";
      bson::BSONObj pattern = BSON(indexName.c_str() << 1);
      bson::BSONObj indexDef = indexTestUtil::createIndexObj(type, indexName.c_str(),
                                                             FALSE, pattern);
      rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
      ASSERT_EQ(SDB_OK, rc);
   }
   {
      string indexName = "index3";
      bson::BSONObj pattern = BSON(indexName.c_str() << 1);
      bson::BSONObj indexDef = indexTestUtil::createIndexObj(type, indexName.c_str(),
                                                             FALSE, pattern);
      rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
      ASSERT_EQ(SDB_OK, rc);
   }

   {
      string indexName = "index3";
      rc = handler->removeIndex(&session, indexName.c_str());
      ASSERT_EQ(SDB_OK, rc);
   }

   rc = handler->listIndex(&session, indexes);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(2, indexes.size());

   handler->close();
   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   {
      string indexName = "index1";
      rc = handler->removeIndex(&session, indexName.c_str());
      ASSERT_EQ(SDB_OK, rc);
   }
   {
      string indexName = "index2";
      rc = handler->removeIndex(&session, indexName.c_str());
      ASSERT_EQ(SDB_OK, rc);
   }
   {
      string indexName = "index3";
      rc = handler->removeIndex(&session, indexName.c_str());
      ASSERT_EQ(SDB_IXM_NOTEXIST, rc);
   }

   indexes.clear();
   rc = handler->listIndex(&session, indexes);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, indexes.size());

   handler->close();
   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   indexes.clear();
   rc = handler->listIndex(&session, indexes);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, indexes.size());

   handler->close();
   db.close(&session, closeDBOptions());
}

TEST_F(index_ddl_test, base_remove_index_test3_1)
{
   remove_index_test3(INDEX_TYPE_HYBRID_TREE);
}

/*
Name: base_create_index_test1
Description: 
   索引重启创建测试
   1. 创建单个cs,cs下创建单个cl
   2. cl创建单个索引
   3. 重启vessel
   4. cl再次创建多个索引,验证索引数量
   5. 再次重启vessel
   6. cl第三次创建多个索引,验证索引数量
Expected Result: 
   索引数量验证正确
*/
void create_index_test1(INDEX_TYPE type)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   DATA_COLLECTION_PTR handler;
   ossPoolVector<bson::BSONObj> indexes;

   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   {
      string indexName = "index1";
      bson::BSONObj pattern = BSON(indexName.c_str() << 1);
      bson::BSONObj indexDef = indexTestUtil::createIndexObj(type, indexName.c_str(),
                                                             FALSE, pattern);
      rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
      ASSERT_EQ(SDB_OK, rc);
   }

   rc = handler->listIndex(&session, indexes);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(1, indexes.size());

   handler->close();
   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   {
      string indexName = "index1";
      bson::BSONObj pattern = BSON(indexName.c_str() << 1);
      bson::BSONObj indexDef = indexTestUtil::createIndexObj(type, indexName.c_str(),
                                                             FALSE, pattern);
      rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
      ASSERT_EQ(SDB_IXM_EXIST, rc);
   }
   {
      string indexName = "index2";
      bson::BSONObj pattern = BSON(indexName.c_str() << 1);
      bson::BSONObj indexDef = indexTestUtil::createIndexObj(type, indexName.c_str(),
                                                             FALSE, pattern);
      rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
      ASSERT_EQ(SDB_OK, rc);
   }

   indexes.clear();
   rc = handler->listIndex(&session, indexes);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(2, indexes.size());

   handler->close();
   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   {
      string indexName = "index1";
      bson::BSONObj pattern = BSON(indexName.c_str() << 1);
      bson::BSONObj indexDef = indexTestUtil::createIndexObj(type, indexName.c_str(),
                                                             FALSE, pattern);
      rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
      ASSERT_EQ(SDB_IXM_EXIST, rc);
   }
   {
      string indexName = "index2";
      bson::BSONObj pattern = BSON(indexName.c_str() << 1);
      bson::BSONObj indexDef = indexTestUtil::createIndexObj(type, indexName.c_str(),
                                                             FALSE, pattern);
      rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
      ASSERT_EQ(SDB_IXM_EXIST, rc);
   }
   {
      string indexName = "index3";
      bson::BSONObj pattern = BSON(indexName.c_str() << 1);
      bson::BSONObj indexDef = indexTestUtil::createIndexObj(type, indexName.c_str(),
                                                             FALSE, pattern);
      rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
      ASSERT_EQ(SDB_OK, rc);
   }

   indexes.clear();
   rc = handler->listIndex(&session, indexes);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(3, indexes.size());

   handler->close();
   db.close(&session, closeDBOptions());
}

TEST_F(index_ddl_test, base_create_index_test1_1)
{
   create_index_test1(INDEX_TYPE_HYBRID_TREE);
}



/*
Name: base_create_index_test2
Description: 
   单个cl最大索引数创建测试
   1. 创建单个cs,cs下创建单个cl
   2. 创建单个cl最大个数的索引,验证索引数量
   3. 删除所有索引,验证索引数量
   4. 再次创建最大个数索引,验证索引数量
   5. 重启vessel，验证索引数量
Expected Result: 
   索引数量验证正确
*/
void create_index_test2(INDEX_TYPE type)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   DATA_COLLECTION_PTR handler;
   ossPoolVector<bson::BSONObj> indexes;

   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   string indexNamePrefix = "index";

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < MAX_INDEX_COUNT_PER_CL; ++i)
   {
      string indexName = indexNamePrefix + to_string(i);
      bson::BSONObj pattern = BSON(indexName.c_str() << 1);
      bson::BSONObj indexDef = indexTestUtil::createIndexObj(type, indexName.c_str(),
                                                             FALSE, pattern);
      rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
      ASSERT_EQ(SDB_OK, rc);
   }

   rc = handler->listIndex(&session, indexes);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(MAX_INDEX_COUNT_PER_CL, indexes.size());

   for (UINT32 i = 0; i < MAX_INDEX_COUNT_PER_CL; ++i)
   {
      string indexName = indexNamePrefix + to_string(i);
      rc = handler->removeIndex(&session, indexName.c_str());
      ASSERT_EQ(SDB_OK, rc);
   }

   indexes.clear();
   rc = handler->listIndex(&session, indexes);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, indexes.size());

   for (UINT32 i = 0; i < MAX_INDEX_COUNT_PER_CL; ++i)
   {
      string indexName = indexNamePrefix + to_string(i);
      bson::BSONObj pattern = BSON(indexName.c_str() << 1);
      bson::BSONObj indexDef = indexTestUtil::createIndexObj(type, indexName.c_str(),
                                                             FALSE, pattern);
      rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
      ASSERT_EQ(SDB_OK, rc);
   }

   indexes.clear();
   rc = handler->listIndex(&session, indexes);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(MAX_INDEX_COUNT_PER_CL, indexes.size());

   handler->close();
   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   indexes.clear();
   rc = handler->listIndex(&session, indexes);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(MAX_INDEX_COUNT_PER_CL, indexes.size());

   handler->close();
   db.close(&session, closeDBOptions());
}

TEST_F(index_ddl_test, base_create_index_test2_1)
{
   create_index_test2(INDEX_TYPE_HYBRID_TREE);
}


/*
Name: advanced_create_index_test1
Description: 
   多线程索引创建与删除
   1. 创建单个cs
   2. 启动多个线程,每个线程创建一个cl，并创建多个索引
   3. 验证cl数量和索引数量
   4. 重启vessel,验证索引数量
   5. 启动多个线程,删除各个cl下的所有索引
   6. 验证索引数量
Expected Result: 
   索引数量验证正确
*/
void thread_create(vesselImpl *db,
                   INDEX_TYPE type,
                   const string &fullPrefix,
                   const string &indexPrefix,
                   UINT32 indexCount,
                   UINT32 x)
{
   INT32 rc = SDB_OK;
   test_executor session;
   string fullName = fullPrefix + to_string(x);
   DATA_COLLECTION_PTR handler;

   rc = db->createCL(&session, fullName.c_str(), x, 
                     dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db->openCL(&session, fullName.c_str(), dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < indexCount; ++i)
   {
      string indexName = indexPrefix + to_string(i);
      bson::BSONObj pattern = BSON(indexName.c_str() << 1);
      bson::BSONObj indexDef = indexTestUtil::createIndexObj(type, indexName.c_str(),
                                                             FALSE, pattern);
      rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
      ASSERT_EQ(SDB_OK, rc);
   }
   handler->close();
}

void thread_remove(vesselImpl *db,
                   INDEX_TYPE type,
                   const string &fullPrefix,
                   const string &indexPrefix,
                   UINT32 indexCount,
                   UINT32 x)
{
   INT32 rc = SDB_OK;
   test_executor session;
   string fullName = fullPrefix + to_string(x);
   DATA_COLLECTION_PTR handler;

   rc = db->openCL(&session, fullName.c_str(), dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < indexCount; ++i)
   {
      string indexName = indexPrefix + to_string(i);
      rc = handler->removeIndex(&session, indexName.c_str());
      ASSERT_EQ(SDB_OK, rc);
   }
   handler->close();
}

void advanced_create_test1(INDEX_TYPE type)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   DATA_COLLECTION_PTR handler;
   ossPoolVector<bson::BSONObj> indexes;

   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   constexpr UINT32 threadCount = 2;
   thread threads[threadCount];
   string fullNamePrefix = "foo.bar";
   string indexNamePrefix = "index";
   UINT32 indexCount = 4;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(thread_create, &db, type,
                                         fullNamePrefix, indexNamePrefix,
                                         indexCount, i));
   }
   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i].join();
   }

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      string fullName = fullNamePrefix + to_string(i);
      rc = db.openCL(&session, fullName.c_str(), dmsOpenCLOptions(), handler);
      ASSERT_EQ(SDB_OK, rc);
      
      indexes.clear();
      rc = handler->listIndex(&session, indexes); 
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(indexCount, indexes.size());
      handler->close();
   }

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(thread_remove, &db, type,
                                         fullNamePrefix, indexNamePrefix,
                                         indexCount, i));
   }
   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i].join();
   }

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      string fullName = fullNamePrefix + to_string(i);
      rc = db.openCL(&session, fullName.c_str(), dmsOpenCLOptions(), handler);
      ASSERT_EQ(SDB_OK, rc);
      
      indexes.clear();
      rc = handler->listIndex(&session, indexes); 
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(0, indexes.size());
      handler->close();
   }

   db.close(&session, closeDBOptions());
}

TEST_F(index_ddl_test, advanced_create_index_test1_1)
{
   advanced_create_test1(INDEX_TYPE_HYBRID_TREE);
}


/*
Name: advanced_create_index_test2
Description: 
   多线程索引冲突创建与删除
   1. 创建单个cs,cs下创建单个cl
   2. 启动多个线程, 在cl下并发创建多个索引
   3. 验证索引创建数量
   4. 重启vessel
   5. 启动多个线程，并发删除多个索引
   6. 验证索引数量
Expected Result: 
   索引数量验证正确
*/
void thread_interference_create(vesselImpl *db,
                                INDEX_TYPE type,
                                const string &fullName,
                                const string &indexPrefix,
                                UINT32 indexCount)
{
   INT32 rc = SDB_OK;
   test_executor session;
   DATA_COLLECTION_PTR handler;

   rc = db->openCL(&session, fullName.c_str(), dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < indexCount; ++i)
   {
      string indexName = indexPrefix + to_string(i);
      bson::BSONObj pattern = BSON(indexName.c_str() << 1);
      bson::BSONObj indexDef = indexTestUtil::createIndexObj(type, indexName.c_str(),
                                                             FALSE, pattern);
      rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
      if (SDB_OK == rc || SDB_IXM_EXIST == rc)
      {
         continue;
      }
      else
      {
         ASSERT_EQ(SDB_OK, rc);
      }
   }
   handler->close();
}

void thread_interference_remove(vesselImpl *db,
                                INDEX_TYPE type,
                                const string &fullName,
                                const string &indexPrefix,
                                UINT32 indexCount)
{
   INT32 rc = SDB_OK;
   test_executor session;

   DATA_COLLECTION_PTR handler;

   rc = db->openCL(&session, fullName.c_str(), dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < indexCount; ++i)
   {
      string indexName = indexPrefix + to_string(i);
      rc = handler->removeIndex(&session, indexName.c_str());
      if (SDB_OK == rc || 
          SDB_IXM_NOTEXIST == rc ||
          SDB_INVALID_OPERATION == rc)
      {
         continue;
      }
      else
      {
         ASSERT_EQ(SDB_OK, rc);
      }
   }
   handler->close();
}

void advanced_create_test2(INDEX_TYPE type)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   DATA_COLLECTION_PTR handler;
   ossPoolVector<bson::BSONObj> indexes;

   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   constexpr UINT32 threadCount = 2;
   thread threads[threadCount];
   string fullName = "foo.bar";
   string indexNamePrefix = "index";
   UINT32 indexCount = 32;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, fullName.c_str(), 1, 
                    dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(thread_interference_create, &db, type,
                                         fullName, indexNamePrefix,
                                         indexCount));
   }
   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i].join();
   }

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, fullName.c_str(), dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);
   
   indexes.clear();
   rc = handler->listIndex(&session, indexes); 
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(indexCount, indexes.size());
   handler->close();
   
   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(thread_interference_remove, &db, type,
                                         fullName, indexNamePrefix, indexCount));
   }
   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i].join();
   }

   rc = db.openCL(&session, fullName.c_str(), dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);
   
   indexes.clear();
   rc = handler->listIndex(&session, indexes); 
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(0, indexes.size());
   handler->close();
   
   db.close(&session, closeDBOptions());
}

TEST_F(index_ddl_test, advanced_create_index_test2_1)
{
   advanced_create_test2(INDEX_TYPE_HYBRID_TREE);
}

/*
Name: advanced_create_index_test3
Description: 
   多个cl创建多个索引
   1. 创建单个cs
   2. cs下创建1000个cl
   3. 每个cl创建最大数量索引
   4. 验证索引和cl数量是否符合预期
   5. 重启vessel，验证索引索引和cl数量
Expected Result: 
   索引和cl数量验证正确
*/
TEST_F(index_ddl_test, advanced_create_index_test3)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   DATA_COLLECTION_PTR handler;
   ossPoolVector<bson::BSONObj> indexes;

   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   string fullNamePrefix = "foo.bar";
   string indexNamePrefix = "index";
   constexpr INT32 createdCLNum = 1000;
   UINT16 clCount = 0;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   for (INT32 i = 0; i < createdCLNum; ++i)
   {
      string fullName = fullNamePrefix + to_string(i);
      rc = db.createCL(&session, fullName.c_str(), i, dmsCreateCLOptions(), bson::BSONObj());
      if (SDB_OK == rc)
      {
         ++clCount;
      }
      else if (SDB_NOSPC == rc)
      {
         rc = SDB_OK;
         break;
      }
      else
      {
         ASSERT_EQ(SDB_NOSPC, rc);
      }

      rc = db.openCL(&session, fullName.c_str(), dmsOpenCLOptions(), handler);
      ASSERT_EQ(SDB_OK, rc);

      UINT32 indexCount = 0;
      for (UINT32 i = 0; i < MAX_INDEX_COUNT_PER_CL; ++i)
      {
         string indexName = indexNamePrefix + to_string(i);
         bson::BSONObj pattern = BSON(indexName.c_str() << 1);
         bson::BSONObj indexDef = indexTestUtil::createIndexObj(INDEX_TYPE_HYBRID_TREE, 
                                                                indexName.c_str(),
                                                                FALSE, pattern);
         rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
         if (SDB_OK == rc)
         {
            ++indexCount;
         }
         else if (SDB_NOSPC == rc)
         {
            rc = SDB_OK;
            break;
         }
         else
         {
            ASSERT_EQ(SDB_NOSPC, rc);
         }
      }

      indexes.clear();
      rc = handler->listIndex(&session, indexes); 
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(indexCount, indexes.size());
      handler->close();
   }

   DATA_CURSOR_PTR cursor;
   rc = db.listCL(&session, "foo", cursor);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < clCount; ++i)
   {
      rc = cursor->fetchNext(&session);
      ASSERT_EQ(SDB_OK, rc);
   }
   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_DMS_EOC, rc);
   

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   for (INT32 i = 0; i < createdCLNum; ++i)
   {
      string fullName = fullNamePrefix + to_string(i);

      rc = db.openCL(&session, fullName.c_str(), dmsOpenCLOptions(), handler);
      ASSERT_EQ(SDB_OK, rc);

      indexes.clear();
      rc = handler->listIndex(&session, indexes); 
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(MAX_INDEX_COUNT_PER_CL, indexes.size());

      string indexName = "index";
      bson::BSONObj pattern = BSON(indexName.c_str() << 1);
      bson::BSONObj indexDef = indexTestUtil::createIndexObj(INDEX_TYPE_HYBRID_TREE, indexName.c_str(),
                                                             FALSE, pattern);
      rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
      ASSERT_EQ(SDB_DMS_MAX_INDEX, rc);
      handler->close();
   }

   db.close(&session, closeDBOptions());
   
}

/*
Name: death_create_index_test1
Description: 
   最大数量cl最大索引数创建测试
   1. 创建单个cs
   2. cs下创建最大数量cl
   3. 每个cl创建最大数量索引
   4. 验证索引和cl数量是否符合预期
   5. 重启vessel，验证索引索引和cl数量
Expected Result: 
   索引和cl数量验证正确
*/
TEST_F(index_ddl_test, DISABLED_death_create_index_test1)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   DATA_COLLECTION_PTR handler;
   ossPoolVector<bson::BSONObj> indexes;

   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   string fullNamePrefix = "foo.bar";
   string indexNamePrefix = "index";
   UINT16 clCount = 0;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   for (INT32 i = 0; i < MAX_CL_MB_COUNT; ++i)
   {
      string fullName = fullNamePrefix + to_string(i);
      rc = db.createCL(&session, fullName.c_str(), i, dmsCreateCLOptions(), bson::BSONObj());
      if (SDB_OK == rc)
      {
         ++clCount;
      }
      else if (SDB_NOSPC == rc)
      {
         rc = SDB_OK;
         break;
      }
      else
      {
         ASSERT_EQ(SDB_NOSPC, rc);
      }

      rc = db.openCL(&session, fullName.c_str(), dmsOpenCLOptions(), handler);
      ASSERT_EQ(SDB_OK, rc);

      UINT32 indexCount = 0;
      for (UINT32 i = 0; i < MAX_INDEX_COUNT_PER_CL; ++i)
      {
         string indexName = indexNamePrefix + to_string(i);
         bson::BSONObj pattern = BSON(indexName.c_str() << 1);
         bson::BSONObj indexDef = indexTestUtil::createIndexObj(INDEX_TYPE_HYBRID_TREE, indexName.c_str(),
                                                               FALSE, pattern);
         rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
         if (SDB_OK == rc)
         {
            ++indexCount;
         }
         else if (SDB_NOSPC == rc)
         {
            rc = SDB_OK;
            break;
         }
         else
         {
            ASSERT_EQ(SDB_NOSPC, rc);
         }
      }

      indexes.clear();
      rc = handler->listIndex(&session, indexes); 
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(indexCount, indexes.size());
      handler->close();
   }

   DATA_CURSOR_PTR cursor;
   rc = db.listCL(&session, "foo", cursor);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < clCount; ++i)
   {
      rc = cursor->fetchNext(&session);
      ASSERT_EQ(SDB_OK, rc);
   }
   rc = cursor->fetchNext(&session);
   ASSERT_EQ(SDB_DMS_EOC, rc);
   

   rc = db.close(&session, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   for (INT32 i = 0; i < MAX_CL_MB_COUNT; ++i)
   {
      string fullName = fullNamePrefix + to_string(i);

      rc = db.openCL(&session, fullName.c_str(), dmsOpenCLOptions(), handler);
      ASSERT_EQ(SDB_OK, rc);

      indexes.clear();
      rc = handler->listIndex(&session, indexes); 
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(MAX_INDEX_COUNT_PER_CL, indexes.size());

      string indexName = "index";
      bson::BSONObj pattern = BSON(indexName.c_str() << 1);
      bson::BSONObj indexDef = indexTestUtil::createIndexObj(INDEX_TYPE_HYBRID_TREE, indexName.c_str(),
                                                            FALSE, pattern);
      rc = handler->createIndex(&session, dmsBuildIndexOptions(), indexDef);
      ASSERT_EQ(SDB_DMS_MAX_INDEX, rc);
      handler->close();
   }

   db.close(&session, closeDBOptions());
}

