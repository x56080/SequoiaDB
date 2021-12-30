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

   Source File Name = index_ddl_test.cpp

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
#include "vessel/requestContext.h"
#include <gtest/gtest.h>
#include "ossUtil.hpp"
#include "vessel/IRedoLogger.h"
#include "vessel/logRecordContext.h"
#include "dpsLogRecord.hpp"
#include "../bson/bson.hpp"
#include "pd.hpp"
#include "vessel/collectionOptions.h"
#include "mthMatchTree.hpp"
#include <iostream>
#include "dmsCursorReader.hpp"

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

   indexParameters params;
   params.type = type;

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
      bson::BSONObj indexDef = indexTestUtil::createIndexObj(indexName.c_str(),
                                                             params, pattern);
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
      bson::BSONObj indexDef = indexTestUtil::createIndexObj(indexName.c_str(),
                                                             params, pattern);
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
      bson::BSONObj indexDef = indexTestUtil::createIndexObj(indexName.c_str(),
                                                             params, pattern);
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
      bson::BSONObj indexDef = indexTestUtil::createIndexObj(indexName.c_str(),
                                                             params, pattern);
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
   remove_index1(INDEX_TYPE_BTREE);
}

TEST_F(index_ddl_test, base_remove_1_2)
{
   remove_index1(INDEX_TYPE_LSM);
}

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

   indexParameters params;
   params.type = type;

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
      bson::BSONObj indexDef = indexTestUtil::createIndexObj(indexName,
                                                            params, pattern);
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
   remove_index2(INDEX_TYPE_BTREE);
}

TEST_F(index_ddl_test, base_remove_2_2)
{
   remove_index2(INDEX_TYPE_LSM);
}