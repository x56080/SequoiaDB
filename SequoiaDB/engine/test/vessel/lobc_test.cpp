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

   Source File Name = lobc_test.cpp

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
#include "vessel/memoryBlock.h"
#include <gtest/gtest.h>
#include "../bson/bson.hpp"
#include <boost/filesystem.hpp>
#include "dmsLobDef.hpp"


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
   std::list<bson::OID> oids;
   constexpr UINT32 LOBC_COUNT = 10 * 1024 *1024;


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


/// truncate
TEST_F(lobc_test, base_test4)
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
   std::list<bson::OID> oids;
   constexpr UINT32 LOBC_COUNT = 1024 * 1024;


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

   rc = handler->truncate(&executor, dmsTruncateCLOptions());
   ASSERT_EQ(SDB_OK, rc);

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

/// unaligned chunk
TEST_F(lobc_test, base_test5)
{
   INT32 rc = SDB_OK;

   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   test_executor executor;

   constexpr UINT32 LOBC_SIZE = 10000;
   CHAR buf[LOBC_SIZE];
   ossMemset(buf, 0xFF, LOBC_SIZE);
   DATA_COLLECTION_PTR handler;
   std::vector<bson::OID> oids;
   constexpr UINT32 LOBC_COUNT = 1024 *1024;


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

/// update
TEST_F(lobc_test, base_test6)
{
   INT32 rc = SDB_OK;

   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   test_executor executor;

   constexpr UINT32 UPDATE_SIZE = 1000;
   constexpr UINT32 UPDATE_TIMES = MAX_LOB_CHUNK_SIZE / UPDATE_SIZE;
   constexpr UINT32 TOTAL_LOBC_SIZE = UPDATE_SIZE * UPDATE_TIMES;
   memoryBlock mb;
   rc = mb.reserve(TOTAL_LOBC_SIZE);
   ASSERT_EQ(SDB_OK, rc);
   
   CHAR buf[UPDATE_SIZE];
   DATA_COLLECTION_PTR handler;
   std::list<bson::OID> oids;
   constexpr UINT32 LOBC_COUNT = 10;

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
      UINT32 offset = 0;
      for (UINT32 j = 0; j < UPDATE_TIMES; ++j)
      {
         ossMemset(buf, (CHAR)j, UPDATE_SIZE);
         rc = handler->updateLobChunk(&executor, oid, 0,
                                      offset, UPDATE_SIZE,
                                      buf, TRUE);
         ASSERT_EQ(SDB_OK, rc);
         offset += UPDATE_SIZE;
      }
      oids.push_back(oid);
   }

   for (auto const &oid : oids)
   {
      UINT32 readSize = 0;
      rc = handler->readLobChunk(&executor, oid, 0, 0, TOTAL_LOBC_SIZE,
                                 mb.getBuffer(), readSize);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(TOTAL_LOBC_SIZE, readSize);
      for (UINT32 i = 0; i < UPDATE_TIMES; ++i)
      {
         ossMemset(buf, (CHAR)i, UPDATE_SIZE);
         CHAR *tmp = mb.getBuffer() + (i * UPDATE_SIZE);
         INT32 cmp = ossMemcmp(tmp, buf, UPDATE_SIZE);
         ASSERT_EQ(0, cmp);
      }
      ossMemset(mb.getBuffer(), 0x00, mb.getCapacity());
   }   

   rc = db.close(&executor, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

TEST_F(lobc_test, base_truncate_1)
{
   INT32 rc = SDB_OK;

   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   test_executor executor;

   constexpr UINT32 LOBC_SIZE = 1 << 20;
   memoryBlock mb, readBuffer;
   rc = mb.reserve(LOBC_SIZE);
   ASSERT_EQ(SDB_OK, rc);
   rc = readBuffer.reserve(LOBC_SIZE);
   ASSERT_EQ(SDB_OK, rc);
   ossMemset(mb.getBuffer(), 0xFF, LOBC_SIZE);
   
   DATA_COLLECTION_PTR handler;
   std::list<bson::OID> oids;
   constexpr UINT32 LOBC_COUNT = 1024 * 8;

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

   UINT32 size0 = LOBC_SIZE / 2;
   for (auto const &oid : oids)
   {
      UINT32 tsize = 0;
      rc = handler->truncateLobChunk(&executor, oid, 0, size0, tsize);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(tsize, LOBC_SIZE - size0);
   }

   for (auto const &oid : oids)
   {
      UINT32 rsize = 0;
      rc = handler->readLobChunk(&executor, oid, 0, 0,
                                 LOBC_SIZE, readBuffer.getBuffer(), rsize);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(rsize, size0);
   }  

   UINT32 size1 = LOBC_SIZE / 4;
   for (auto const &oid : oids)
   {
      UINT32 tsize = 0;
      rc = handler->truncateLobChunk(&executor, oid, 0, size1, tsize);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(tsize, size0 - size1);
   }

   for (auto const &oid : oids)
   {
      UINT32 rsize = 0;
      rc = handler->readLobChunk(&executor, oid, 0, 0,
                                 LOBC_SIZE, readBuffer.getBuffer(), rsize);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(rsize, size1);
   }

   rc = db.close(&executor, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

TEST_F(lobc_test, base_truncate_2)
{
   INT32 rc = SDB_OK;

   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   test_executor executor;

   constexpr UINT32 UPDATE_SIZE = 4096;
   constexpr UINT32 UPDATE_TIMES = MAX_LOB_CHUNK_SIZE / UPDATE_SIZE;
   constexpr UINT32 LOBC_SIZE = UPDATE_SIZE * UPDATE_TIMES;
   memoryBlock readBuffer;
   rc = readBuffer.reserve(LOBC_SIZE);
   ASSERT_EQ(SDB_OK, rc);
   
   CHAR buf[UPDATE_SIZE];
   DATA_COLLECTION_PTR handler;
   std::list<bson::OID> oids;
   constexpr UINT32 LOBC_COUNT = 100;

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
      UINT32 offset = 0;
      for (UINT32 j = 0; j < UPDATE_TIMES; ++j)
      {
         ossMemset(buf, (CHAR)j, UPDATE_SIZE);
         rc = handler->updateLobChunk(&executor, oid, 0,
                                      offset, UPDATE_SIZE,
                                      buf, TRUE);
         ASSERT_EQ(SDB_OK, rc);
         offset += UPDATE_SIZE;
      }
      oids.push_back(oid);
   }

   for (auto const &oid : oids)
   {
      dmsLobChunkProfile profile;
      rc = handler->testLobChunk(&executor, oid, 0, &profile);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(profile.chunkSize, LOBC_SIZE);
   }

   UINT32 size0 = LOBC_SIZE / 2;
   for (auto const &oid : oids)
   {
      UINT32 tsize = 0;
      rc = handler->truncateLobChunk(&executor, oid, 0, size0, tsize);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(tsize, LOBC_SIZE - size0);
   }

   for (auto const &oid : oids)
   {
      UINT32 rsize = 0;
      rc = handler->readLobChunk(&executor, oid, 0, 0,
                                 LOBC_SIZE, readBuffer.getBuffer(), rsize);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(rsize, size0);
   }  

   UINT32 size1 = LOBC_SIZE / 4 - 1;
   for (auto const &oid : oids)
   {
      UINT32 tsize = 0;
      rc = handler->truncateLobChunk(&executor, oid, 0, size1, tsize);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(tsize, size0 - size1);
   }

   for (auto const &oid : oids)
   {
      UINT32 rsize = 0;
      rc = handler->readLobChunk(&executor, oid, 0, 0,
                                 LOBC_SIZE, readBuffer.getBuffer(), rsize);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(rsize, size1);
   }

   rc = db.close(&executor, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

TEST_F(lobc_test, base_list_test1)
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

   constexpr UINT32 LOBC_COUNT = 10 * 1024 *1024;


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
   }

   DATA_CURSOR_PTR cursor;
   rc = handler->listLobChunks(&executor, dmsListLobChunkOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);
   for (UINT32 i = 0; i < LOBC_COUNT; ++i)
   {
      rc = cursor->fetchNext(&executor);
      ASSERT_EQ(SDB_OK, rc);
      const dmsLobChunkInfo *info = cursor->getDataObjPtr<dmsLobChunkInfo>();
      ASSERT_NE(nullptr, info);
      ASSERT_TRUE(info->oid.isSet());
      ASSERT_EQ(0, info->chunkId);
      ASSERT_EQ(1, info->profile.chainSize);
      ASSERT_EQ(LOBC_SIZE, info->profile.chunkSize);
      ASSERT_EQ(0, info->profile.flags);
   }

   rc = cursor->fetchNext(&executor);
   ASSERT_EQ(SDB_DMS_EOC, rc);
   cursor->close();

   rc = db.close(&executor, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.openCL(&session, "foo.bar", dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   rc = handler->listLobChunks(&executor, dmsListLobChunkOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);
   for (UINT32 i = 0; i < LOBC_COUNT; ++i)
   {
      rc = cursor->fetchNext(&executor);
      ASSERT_EQ(SDB_OK, rc);
      const dmsLobChunkInfo *info = cursor->getDataObjPtr<dmsLobChunkInfo>();
      ASSERT_NE(nullptr, info);
      ASSERT_TRUE(info->oid.isSet());
      ASSERT_EQ(0, info->chunkId);
      ASSERT_EQ(1, info->profile.chainSize);
      ASSERT_EQ(LOBC_SIZE, info->profile.chunkSize);
      ASSERT_EQ(0, info->profile.flags);
   }

   rc = cursor->fetchNext(&executor);
   ASSERT_EQ(SDB_DMS_EOC, rc);
   cursor->close();

   rc = db.close(&executor, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

TEST_F(lobc_test, base_list_test2)
{
   INT32 rc = SDB_OK;

   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   test_executor executor;

   constexpr UINT32 UPDATE_SIZE = 4096;
   constexpr UINT32 UPDATE_TIMES = MAX_LOB_CHUNK_SIZE / UPDATE_SIZE;
   constexpr UINT32 LOBC_SIZE = UPDATE_SIZE * UPDATE_TIMES;
   memoryBlock readBuffer;
   rc = readBuffer.reserve(LOBC_SIZE);
   ASSERT_EQ(SDB_OK, rc);
   
   CHAR buf[UPDATE_SIZE];
   DATA_COLLECTION_PTR handler;
   std::set<bson::OID> oids;
   constexpr UINT32 LOBC_COUNT = 100;

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
      UINT32 offset = 0;
      for (UINT32 j = 0; j < UPDATE_TIMES; ++j)
      {
         ossMemset(buf, (CHAR)j, UPDATE_SIZE);
         rc = handler->updateLobChunk(&executor, oid, 0,
                                      offset, UPDATE_SIZE,
                                      buf, TRUE);
         ASSERT_EQ(SDB_OK, rc);
         offset += UPDATE_SIZE;
      }
      oids.insert(oid);
   }

   DATA_CURSOR_PTR cursor;
   rc = handler->listLobChunks(&executor, dmsListLobChunkOptions(), cursor);
   ASSERT_EQ(SDB_OK, rc);
   for (UINT32 i = 0; i < LOBC_COUNT; ++i)
   {
      rc = cursor->fetchNext(&executor);
      ASSERT_EQ(SDB_OK, rc);
      const dmsLobChunkInfo *info = cursor->getDataObjPtr<dmsLobChunkInfo>();
      ASSERT_NE(nullptr, info);
      ASSERT_TRUE(info->oid.isSet());
      ASSERT_EQ(0, info->chunkId);
      ASSERT_EQ(UPDATE_TIMES, info->profile.chainSize);
      ASSERT_EQ(LOBC_SIZE, info->profile.chunkSize);
      ASSERT_EQ(0, info->profile.flags);
      ASSERT_EQ(1, oids.count(info->oid));
   }

   rc = cursor->fetchNext(&executor);
   ASSERT_EQ(SDB_DMS_EOC, rc);
   cursor->close();

   rc = db.close(&executor, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}

void func_multi_cl_same_oid(vesselImpl *db, UINT32 i, const std::list<bson::OID> &oids)
{
   INT32 rc = SDB_OK;
   std::stringstream ss;
   ss << "foo.bar" << i;
   std::string fullName = ss.str();
   test_executor executor;
   constexpr UINT32 LOBC_SIZE = 50000;
   CHAR buf[LOBC_SIZE];
   DATA_COLLECTION_PTR handler;

   rc = db->createCL(&executor, fullName.c_str(), i + 1,
                     dmsCreateCLOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   rc = db->openCL(&executor, fullName.c_str(), dmsOpenCLOptions(), handler);
   ASSERT_EQ(SDB_OK, rc);

   for (auto const &oid : oids)
   {
      UINT8 r = ossRand();
      ossMemset(buf, r, LOBC_SIZE);
      rc = handler->insertLobChunk(&executor, oid, 0, 0, LOBC_SIZE, buf);
      ASSERT_EQ(SDB_OK, rc);

      CHAR readBuf[LOBC_SIZE] = {};
      UINT32 readSize = 0;
      rc = handler->readLobChunk(&executor, oid, 0, 0, LOBC_SIZE, readBuf, readSize);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(LOBC_SIZE, readSize);
      INT32 cmp = ossMemcmp(buf, readBuf, LOBC_SIZE);
      ASSERT_EQ(0, cmp);
   }

   for (auto const &oid : oids)
   {
      CHAR readBuf[LOBC_SIZE] = {};
      UINT32 readSize = 0;
      rc = handler->readLobChunk(&executor, oid, 0, 0, LOBC_SIZE, readBuf, readSize);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(LOBC_SIZE, readSize);
   }

   handler->close();
}

/// multi cl, same oid
TEST_F(lobc_test, advanced_test1)
{
   vesselImpl db;
   outerResource resource = test_outer_resource::getResource();
   test_executor session;
   openDBOptions options;
   options.path.dataPath = DATA_PATH;
   options.path.lsmPath = LSM_PATH;
   test_executor executor;
   DATA_COLLECTION_PTR handler;
   INT32 rc = SDB_OK;

   rc = db.open(&session, &resource, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = db.createCS(&session, "foo", 1, dmsCreateCSOptions(), bson::BSONObj());
   ASSERT_EQ(SDB_OK, rc);

   constexpr UINT32 OID_COUNT = 300 * 1024;
   std::list<bson::OID> oids;
   for (UINT32 i = 0; i < OID_COUNT; ++i)
   {
      oids.push_back(bson::OID::gen());
   }

   constexpr UINT32 THREAD_COUNT = 4;
   std::thread threads[THREAD_COUNT];
   for (UINT32 i = 0; i < THREAD_COUNT; ++i)
   {
      threads[i] = std::move(std::thread(func_multi_cl_same_oid,
                                         &db, i, oids));
   }

   for (UINT32 i = 0; i < THREAD_COUNT; ++i)
   {
      threads[i].join();
   }

   rc = db.close(&executor, closeDBOptions());
   ASSERT_EQ(SDB_OK, rc);
}