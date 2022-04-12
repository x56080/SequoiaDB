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

   Source File Name = lobc_test.cpp

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
#include "vessel/memoryBlock.h"
#include <gtest/gtest.h>
#include "../bson/bson.hpp"
#include <boost/filesystem.hpp>


namespace fs = boost::filesystem;


class lobc_test : public testing::Test
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

/// small chunk
TEST_F(lobc_test, base_test1)
{
   INT32 rc = SDB_OK;

   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   test_executor executor;

   constexpr UINT32 LOBC_SIZE = 1024;
   CHAR buf[LOBC_SIZE];
   ossMemset(buf, 0xFF, LOBC_SIZE);
   DATA_COLLECTION_PTR handler;
   std::vector<bson::OID> oids;
   constexpr UINT32 LOBC_COUNT = 1024;


   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < LOBC_COUNT; ++i)
   {
      bson::OID oid;
      oid.init();
      rc = handler->insertLobChunk(&executor, oid, 0, 0, LOBC_SIZE, buf);
      ASSERT_EQ(SDB_OK, rc);
      oids.push_back(oid);
   }

   for (auto const &oid : oids)
   {
      UINT32 readSize = 0;
      CHAR readBuf[LOBC_SIZE] = {};
      rc = handler->readLobChunk(&executor, oid, 0, 0, LOBC_SIZE, readBuf, readSize);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(LOBC_SIZE, readSize);
      INT32 cmp = ossMemcmp(buf, readBuf, LOBC_SIZE);
      ASSERT_EQ(0, cmp);
   }   

   rc = db.close(&executor, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (auto const &oid : oids)
   {
      UINT32 readSize = 0;
      CHAR readBuf[LOBC_SIZE] = {};
      rc = handler->readLobChunk(&executor, oid, 0, 0, LOBC_SIZE, readBuf, readSize);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(LOBC_SIZE, readSize);
      INT32 cmp = ossMemcmp(buf, readBuf, LOBC_SIZE);
      ASSERT_EQ(0, cmp);
   } 

   rc = db.close(&executor, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}


/// max  chunk
TEST_F(lobc_test, base_test2)
{
   INT32 rc = SDB_OK;

   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   test_executor executor;

   constexpr UINT32 LOBC_SIZE = 4 * 1024 * 1024;
   
   memoryBlock mb;
   rc = mb.resize(LOBC_SIZE, 0xFF);
   ASSERT_EQ(SDB_OK, rc);
   DATA_COLLECTION_PTR handler;
   std::vector<bson::OID> oids;
   constexpr UINT32 LOBC_COUNT = 4096;


   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < LOBC_COUNT; ++i)
   {
      bson::OID oid;
      oid.init();
      rc = handler->insertLobChunk(&executor, oid, 0, 0, LOBC_SIZE, mb.getBuffer());
      ASSERT_EQ(SDB_OK, rc);
      oids.push_back(oid);
   }

   memoryBlock readBuffer;
   rc = readBuffer.resize(LOBC_SIZE);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < oids.size(); ++i)
   {
      const bson::OID &oid = oids.at(i);
      UINT32 readSize = 0;
      strictBuffer buffer;
      buffer.makeWritable(readBuffer.getSize(), readBuffer.getBuffer());
      buffer.setBuffer(0x0);
      rc = handler->readLobChunk(&executor, oid, 0, 0, LOBC_SIZE, buffer.getWPtr(), readSize);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(LOBC_SIZE, readSize);
      INT32 cmp = ossMemcmp(buffer.getRPtr(), mb.getBuffer(), LOBC_SIZE);
      ASSERT_EQ(0, cmp);
   }   

   rc = db.close(&executor, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < oids.size(); ++i)
   {
      const bson::OID &oid = oids.at(i);
      UINT32 readSize = 0;
      strictBuffer buffer;
      buffer.makeWritable(readBuffer.getSize(), readBuffer.getBuffer());
      buffer.setBuffer(0x0);
      rc = handler->readLobChunk(&executor, oid, 0, 0, LOBC_SIZE, buffer.getWPtr(), readSize);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(LOBC_SIZE, readSize);
      INT32 cmp = ossMemcmp(buffer.getRPtr(), mb.getBuffer(), LOBC_SIZE);
      ASSERT_EQ(0, cmp);
   } 

   rc = db.close(&executor, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

/// remove
TEST_F(lobc_test, base_test3)
{
   INT32 rc = SDB_OK;

   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   test_executor executor;

   constexpr UINT32 LOBC_SIZE = 1024;
   CHAR buf[LOBC_SIZE];
   ossMemset(buf, 0xFF, LOBC_SIZE);
   DATA_COLLECTION_PTR handler;
   std::vector<bson::OID> oids;
   constexpr UINT32 LOBC_COUNT = 1024;


   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCL(&session, "foo.bar", 1, dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < LOBC_COUNT; ++i)
   {
      bson::OID oid;
      oid.init();
      rc = handler->insertLobChunk(&executor, oid, 0, 0, LOBC_SIZE, buf);
      ASSERT_EQ(SDB_OK, rc);
      oids.push_back(oid);
   }

   for (auto const &oid : oids)
   {
      UINT32 readSize = 0;
      CHAR readBuf[LOBC_SIZE] = {};
      rc = handler->readLobChunk(&executor, oid, 0, 0, LOBC_SIZE, readBuf, readSize);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(LOBC_SIZE, readSize);
      INT32 cmp = ossMemcmp(buf, readBuf, LOBC_SIZE);
      ASSERT_EQ(0, cmp);
   }  

   for (auto const &oid : oids)
   {
      rc = handler->removeLobChunk(&executor, oid, 0);
      ASSERT_EQ(SDB_OK, rc);
   }  

   for (auto const &oid : oids)
   {
      UINT32 readSize = 0;
      CHAR readBuf[LOBC_SIZE] = {};
      rc = handler->readLobChunk(&executor, oid, 0, 0, LOBC_SIZE, readBuf, readSize);
      ASSERT_EQ(SDB_LOB_SEQUENCE_NOT_EXIST, rc);
   }  

   ossMemset(buf, 0xAA, LOBC_SIZE);
   for (auto const &oid : oids)
   {
      rc = handler->insertLobChunk(&executor, oid, 0, 0, LOBC_SIZE, buf);
      ASSERT_EQ(SDB_OK, rc);
   }

   for (auto const &oid : oids)
   {
      UINT32 readSize = 0;
      CHAR readBuf[LOBC_SIZE] = {};
      rc = handler->readLobChunk(&executor, oid, 0, 0, LOBC_SIZE, readBuf, readSize);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(LOBC_SIZE, readSize);
      INT32 cmp = ossMemcmp(buf, readBuf, LOBC_SIZE);
      ASSERT_EQ(0, cmp);
   }  

   rc = db.close(&executor, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (auto const &oid : oids)
   {
      rc = handler->removeLobChunk(&executor, oid, 0);
      ASSERT_EQ(SDB_OK, rc);
   }  

   for (auto const &oid : oids)
   {
      UINT32 readSize = 0;
      CHAR readBuf[LOBC_SIZE] = {};
      rc = handler->readLobChunk(&executor, oid, 0, 0, LOBC_SIZE, readBuf, readSize);
      ASSERT_EQ(SDB_LOB_SEQUENCE_NOT_EXIST, rc);
   }  

   rc = db.close(&executor, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}