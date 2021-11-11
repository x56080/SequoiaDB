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

   Source File Name = index_btree_test.cpp

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

class index_btree_test : public testing::Test
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


TEST_F(index_btree_test, test1)
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
   indexParams.type = INDEX_TYPE_BTREE;
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
      ASSERT_EQ(INDEX_TYPE_BTREE, obj.getIntField(VESSEL_INDEX_FIELD_NAME_TYPE));
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
      ASSERT_EQ(INDEX_TYPE_BTREE, obj.getIntField(VESSEL_INDEX_FIELD_NAME_TYPE));
      std::stringstream ss;
      ss << "index" << i;
      ASSERT_EQ(0, ss.str().compare(obj.getStringField(IXM_NAME_FIELD)));
      ASSERT_EQ(INDEX_STATUS_NORMAL, obj.getIntField(VESSEL_INDEX_FIELD_NAME_STATUS));
   }

   cl.close();
   db.close(&session, closeDBOptions());
}

TEST_F(index_btree_test, test2)
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
   indexParams.type = INDEX_TYPE_BTREE;
   collectionHandler cl;
   bson::BufBuilder builder;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar", 1, clOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCollection(&session, "foo", "bar", openCLOptions(), cl);
   ASSERT_EQ(SDB_OK, rc);

   const CHAR *indexName = "a0";
   strSlice nameSlice(indexName);
   bson::BSONObj patternObj = BSON(indexName << 1);
   rc = cl.createIndex(&session, nameSlice, patternObj, indexParams, indexOptions);
   ASSERT_EQ(SDB_OK, rc);

   insertOptions o;

   for (UINT32 i = 0; i < 10000; ++i)
   {
      utilInsertResult result;
      builder.reset();
      bson::BSONObjBuilder recordBuilder(builder);
      recordBuilder.append(indexName, ossRand());
      bson::BSONObj obj = recordBuilder.done();
      slice record(obj.objsize(), obj.objdata());
      rc = cl.insert(&session, record, INVALID_STRIPING_ID, o, &result);
      ASSERT_EQ(SDB_OK, rc);
   }

   cl.close();
   db.close(&session, closeDBOptions());
   }

   TEST_F(index_btree_test, test3)
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
   indexParams.type = INDEX_TYPE_BTREE;
   collectionHandler cl;
   bson::BufBuilder builder;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollectionSpace(&session, "foo", 1, csOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCollection(&session, "foo", "bar", 1, clOptions);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCollection(&session, "foo", "bar", openCLOptions(), cl);
   ASSERT_EQ(SDB_OK, rc);

   const CHAR *indexName = "a0";
   strSlice nameSlice(indexName);
   bson::BSONObj patternObj = BSON(indexName << 1);
   rc = cl.createIndex(&session, nameSlice, patternObj, indexParams, indexOptions);
   ASSERT_EQ(SDB_OK, rc);

   insertOptions o;
   std::stringstream ss;
   CHAR keyPad[128] = {};
   ossMemset(keyPad, 'a', sizeof(keyPad) - 1);

   for (UINT32 i = 0; i < 100000; ++i)
   {
      UINT32 rand = ossRand();
      ss.str("");
      ss << rand << keyPad;
      utilInsertResult result;
      builder.reset();
      bson::BSONObjBuilder recordBuilder(builder);
      recordBuilder.append(indexName, ss.str().c_str());
      bson::BSONObj obj = recordBuilder.done();
      slice record(obj.objsize(), obj.objdata());
      rc = cl.insert(&session, record, INVALID_STRIPING_ID, o, &result);
      ASSERT_EQ(SDB_OK, rc);
   }

   cl.close();
   db.close(&session, closeDBOptions());
   }