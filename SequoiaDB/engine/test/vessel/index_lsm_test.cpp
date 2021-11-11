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

   Source File Name = index_lsm_test.cpp

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

TEST_F(index_lsm_test, test1)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   createCSOptions csOptions;
   createCLOptions clOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   createIndexOptions indexOptions;
   indexParameters indexParams;
   indexParams.type = INDEX_TYPE_LSM;
   collectionHandler cl;
   
   ossPoolVector<bson::BSONObj> indexes;

   UINT32 count = 64;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar", 1, clOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCollection(&session, "foo", "bar", openCLOptions(), cl);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      std::stringstream ss;
      ss << "index" << i;
      std::string indexName = ss.str();
      strSlice nameSlice(indexName.c_str(), indexName.size());
      bson::BSONObj patternObj = BSON(indexName.c_str() << 1);
      rc = cl.createIndex(&session, nameSlice, patternObj, indexParams, indexOptions);
      ASSERT_EQ(SDB_OK, rc);
   }

   {
      std::stringstream ss;
      ss << "index" << 64;
      std::string indexName = ss.str();
      strSlice nameSlice(indexName.c_str(), indexName.size());
      bson::BSONObj patternObj = BSON(indexName.c_str() << 1);
      rc = cl.createIndex(&session, nameSlice, patternObj, indexParams, indexOptions);
      ASSERT_EQ(SDB_DMS_MAX_INDEX, rc);
   }

   rc = cl.listIndexes(&session, indexes);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(count, indexes.size());

   for (UINT32 i = 0; i < count; ++i)
   {
      const bson::BSONObj &obj = indexes.at(i);
      ASSERT_EQ(INDEX_TYPE_LSM, obj.getIntField(VESSEL_INDEX_FIELD_NAME_TYPE));
      std::stringstream ss;
      ss << "index" << i;
      ASSERT_EQ(0, ss.str().compare(obj.getStringField(IXM_NAME_FIELD)));
      ASSERT_EQ(INDEX_STATUS_NORMAL, obj.getIntField(VESSEL_INDEX_FIELD_NAME_STATUS));
   }

   cl.close();
   db.close(&session, closeDBOptions());

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCollection(&session, "foo", "bar", openCLOptions(), cl);
   ASSERT_EQ(SDB_OK, rc);

   rc = cl.listIndexes(&session, indexes);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(count, indexes.size());

   for (UINT32 i = 0; i < count; ++i)
   {
      const bson::BSONObj &obj = indexes.at(i);
      ASSERT_EQ(INDEX_TYPE_LSM, obj.getIntField(VESSEL_INDEX_FIELD_NAME_TYPE));
      std::stringstream ss;
      ss << "index" << i;
      ASSERT_EQ(0, ss.str().compare(obj.getStringField(IXM_NAME_FIELD)));
      ASSERT_EQ(INDEX_STATUS_NORMAL, obj.getIntField(VESSEL_INDEX_FIELD_NAME_STATUS));
   }

   cl.close();
   db.close(&session, closeDBOptions());
}

TEST_F(index_lsm_test, test2)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   createCSOptions csOptions;
   createCLOptions clOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   createIndexOptions indexOptions;
   indexParameters indexParams;
   indexParams.type = INDEX_TYPE_LSM;
   collectionHandler cl;

   static const UINT32 pad_size = 1024;
   CHAR pad[pad_size] = {};
   
   ossPoolVector<bson::BSONObj> indexes;
   bson::BSONObjBuilder builder;

   UINT32 count = 8;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar", 1, clOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCollection(&session, "foo", "bar", openCLOptions(), cl);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < 200000; ++i)
   {
      utilInsertResult  r;
      builder.reset();
      builder.append("a", i);
      builder.appendBinData("b", pad_size, bson::BinDataType::bdtCustom, pad);
      bson::BSONObj obj = builder.done();
      slice record(obj.objsize(), obj.objdata());
      rc = cl.insert(&session, record, INVALID_STRIPING_ID, insertOptions(), &r);
      ASSERT_EQ(rc, SDB_OK);
   }

   for (UINT32 i = 0; i < count; ++i)
   {
      std::stringstream ss;
      ss << "index" << i;
      std::string indexName = ss.str();
      strSlice nameSlice(indexName.c_str(), indexName.size());
      bson::BSONObj patternObj = BSON(indexName.c_str() << 1);
      rc = cl.createIndex(&session, nameSlice, patternObj, indexParams, indexOptions);
      ASSERT_EQ(SDB_OK, rc);
   }

   indexes.clear();
   rc = cl.listIndexes(&session, indexes);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(count, indexes.size());

   for (UINT32 i = 0; i < count; ++i)
   {
      std::stringstream ss;
      ss << "index" << i;
      const bson::BSONObj &obj = indexes.at(i);
      ASSERT_EQ(INDEX_TYPE_LSM, obj.getIntField(VESSEL_INDEX_FIELD_NAME_TYPE));
      std::string name = ss.str();
      ASSERT_EQ(0, name.compare(obj.getStringField(IXM_NAME_FIELD)));
      ASSERT_EQ(INDEX_STATUS_NORMAL, obj.getIntField(VESSEL_INDEX_FIELD_NAME_STATUS));
   }

   cl.close();
   db.close(&session, closeDBOptions());

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCollection(&session, "foo", "bar", openCLOptions(), cl);
   ASSERT_EQ(SDB_OK, rc);

   indexes.clear();
   rc = cl.listIndexes(&session, indexes);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(count, indexes.size());

   for (UINT32 i = 0; i < count; ++i)
   {
      std::stringstream ss;
      ss << "index" << i;
      const bson::BSONObj &obj = indexes.at(i);
      ASSERT_EQ(INDEX_TYPE_LSM, obj.getIntField(VESSEL_INDEX_FIELD_NAME_TYPE));
      std::string name = ss.str();
      ASSERT_EQ(0, name.compare(obj.getStringField(IXM_NAME_FIELD)));
      ASSERT_EQ(INDEX_STATUS_NORMAL, obj.getIntField(VESSEL_INDEX_FIELD_NAME_STATUS));
   }

   cl.close();
   db.close(&session, closeDBOptions());
}

/// unique index
TEST_F(index_lsm_test, test3)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   createCSOptions csOptions;
   createCLOptions clOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   createIndexOptions indexOptions;
   indexParameters indexParams;
   indexParams.type = INDEX_TYPE_LSM;
   indexParams.isUnique = TRUE;
   collectionHandler cl;
   
   ossPoolVector<bson::BSONObj> indexes;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar", 1, clOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCollection(&session, "foo", "bar", openCLOptions(), cl);
   ASSERT_EQ(SDB_OK, rc);

   std::string indexName("a");
   strSlice nameSlice(indexName.c_str(), indexName.size());
   bson::BSONObj patternObj = BSON(indexName.c_str() << 1);
   rc = cl.createIndex(&session, nameSlice, patternObj, indexParams, indexOptions);
   ASSERT_EQ(SDB_OK, rc);

   bson::BSONObjBuilder builder;
   for (UINT32 i = 0; i < 1000; ++i)
   {
      utilInsertResult res;
      builder.reset();
      builder.append("a", i);
      bson::BSONObj obj = builder.done();
      slice record(obj.objsize(), obj.objdata());
      rc = cl.insert(&session, record,
                     INVALID_STRIPING_ID, insertOptions(), &res);
      ASSERT_EQ(SDB_OK, rc);
   }

   for (UINT32 i = 0; i < 1000; ++i)
   {
      utilInsertResult res;
      builder.reset();
      builder.append("a", i);
      bson::BSONObj obj = builder.done();
      slice record(obj.objsize(), obj.objdata());
      rc = cl.insert(&session, record,
                     INVALID_STRIPING_ID, insertOptions(), &res);
      ASSERT_EQ(SDB_IXM_DUP_KEY, rc);
   }

   cl.close();
   db.close(&session, closeDBOptions());
}

void duplicated_insert(vesselImpl *db,
                       const CHAR *csName, const CHAR *clName,
                       UINT32 count, UINT32 range)
{
   test_executor session;
   bson::BSONObjBuilder builder;
   collectionHandler handler;
   INT32 rc = db->openCollection(&session, csName, clName, openCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      UINT32 r = ossRand() % range;
      builder.reset();
      builder.append("a", r);
      bson::BSONObj obj = builder.done();
      slice record;
      record.reset(obj.objsize(), obj.objdata());
      utilInsertResult res;
      rc = handler.insert(&session, record, 
                          INVALID_STRIPING_ID,
                          insertOptions(), &res);
      ASSERT_TRUE((SDB_OK == rc || SDB_IXM_DUP_KEY == rc));
   }
   handler.close();
}

/// unique index
TEST_F(index_lsm_test, test4)
{
   INT32 rc = SDB_OK;
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   createCSOptions csOptions;
   createCLOptions clOptions;
   options.path.dataPath = DATA_PATH;
   options.path.indexPath = DATA_PATH;
   options.path.lobMetaPath = DATA_PATH;
   options.path.lobPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;

   createIndexOptions indexOptions;
   indexParameters indexParams;
   indexParams.type = INDEX_TYPE_LSM;
   indexParams.isUnique = TRUE;
   collectionHandler cl;

   static const UINT32 threadCount = 6;
   std::thread threads[threadCount];
   UINT32 range = 100;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar", 1, clOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCollection(&session, "foo", "bar", openCLOptions(), cl);
   ASSERT_EQ(SDB_OK, rc);

   std::string indexName("a");
   strSlice nameSlice(indexName.c_str(), indexName.size());
   bson::BSONObj patternObj = BSON(indexName.c_str() << 1);
   rc = cl.createIndex(&session, nameSlice, patternObj, indexParams, indexOptions);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i] = std::move(std::thread(duplicated_insert, &db,
                                         "foo", "bar", 100000, range));
   }

   for (UINT32 i = 0; i < threadCount; ++i)
   {
      threads[i].join();
   }

   UINT64 recordCount = 0;
   rc = cl.getTotalRecordCountInPageHead(&session, recordCount);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_GE(range, recordCount);
   ASSERT_GT(recordCount, 0);

   cl.close();
   db.close(&session, closeDBOptions());
}