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

   Source File Name = rlog_test.cpp

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
#include <gtest/gtest.h>
#include "vessel/redoLogManager.h"
#include "dpsWriteReqBuilder.hpp"
#include "vessel/redoLogFileReader.h"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

using namespace engine::vessel;

class rlog_test : public testing::Test
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
      test_outer_resource::getResource();
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

void active_writer(redoLogManager *rlog)
{
   test_executor executor;
   rlog->writerRun(&executor);
}

constexpr DPS_TAG TAG_RECORD_POS = 1;
constexpr DPS_TAG TAG_PAD_SIZE = 2;
constexpr DPS_TAG TAG_PAD = 3;


void thread_write(redoLogManager *rlog, UINT32 recordNum, UINT64 *resLsn, UINT64 *resOffset)
{
   constexpr UINT32 PAD_SIZE = 128 << 10;
   std::unique_ptr<CHAR []> pad(new CHAR[PAD_SIZE]);
   test_executor executor;
   INT32 rc = SDB_OK;
   UINT64 offset = 0;
   UINT64 lsn = 0;
   
   for (UINT32 i = 0; i < recordNum; ++i)
   {
      dpsLogRecordHeader jres;
      UINT32 size = (ossRand() % PAD_SIZE) + 1;
      dpsWriteReqBuilder builder;
      builder.setType(LOG_TYPE_DATA_INSERT);
      rc = builder.appendNumeric(TAG_RECORD_POS, i);
      ASSERT_EQ(SDB_OK, rc);
      rc = builder.appendNumeric(TAG_PAD_SIZE, size);
      ASSERT_EQ(SDB_OK, rc);
      rc = builder.append(TAG_PAD, size, pad.get());
      ASSERT_EQ(SDB_OK, rc);
      rc = rlog->write(&executor, builder.reap(), dpsWriteOptions(), &jres);
      ASSERT_EQ(SDB_OK, rc);
      offset = jres._lsn + jres._length;
      lsn = jres._lsn;
   }

   if (nullptr != resLsn)
   {
      *resLsn = lsn;
   }

   if (nullptr != resOffset)
   {
      *resOffset = offset;
   }

   return;
}

TEST_F(rlog_test, base_test1)
{
   test_executor executor;
   redoLogManager rlog;
   redoLogOptions o;
   INT32 rc = rlog.init(DATA_PATH, o, 0);
   ASSERT_EQ(SDB_OK, rc);
   UINT64 offset = 0;
   UINT64 lsn = 0;
   utilUniqueBuffer buffer;
   const UINT32 recordNum = 100000;
   UINT32 fetched = 0;
   std::thread writer = std::move(std::thread(active_writer, &rlog));

   thread_write(&rlog, recordNum, &lsn, &offset);

   rc = rlog.flush(DPS_INVALID_LSN_OFFSET, FALSE);
   ASSERT_EQ(SDB_OK, rc);
   rlog.fini();
   writer.join();

   /// read
   rc = rlog.init(DATA_PATH, o, offset);
   ASSERT_EQ(SDB_OK, rc);

   ASSERT_EQ(lsn, rlog.getCurrentLsnOffset());
   ASSERT_EQ(offset, rlog.getExpectedLsnOffset());

   ossPoolVector<const redoLogFile *> files;
   rc = rlog.exportFilesToScan(files);
   ASSERT_EQ(SDB_OK, rc);

   for (auto file : files)
   {
      redoLogFileReader reader;
      rc = reader.open(file, DPS_INVALID_LSN_OFFSET);
      ASSERT_EQ(SDB_OK, rc);
      while (reader.isReady())
      {
         if (reader.getRecordHeader()._type == LOG_TYPE_DATA_INSERT)
         {
            ++fetched;
            dpsRecordElements elements;
            rc = reader.getElements(&buffer, elements);
            ASSERT_EQ(SDB_OK, rc);

            dpsRecordElements::iterator itr = elements.seek(TAG_PAD_SIZE);
            ASSERT_TRUE(itr.isValid());
            UINT32 size = *(itr.getValue().castTo<UINT32>());
            itr = elements.seek(TAG_PAD);
            ASSERT_TRUE(itr.isValid());
            ASSERT_EQ(size, itr.getValue().size());
         }

         rc = reader.next();
         ASSERT_EQ(SDB_OK, rc);
      }
   }

   ASSERT_EQ(recordNum, fetched);
}

TEST_F(rlog_test, base_test2)
{
   test_executor executor;
   redoLogManager rlog;
   redoLogOptions o;
   INT32 rc = rlog.init(DATA_PATH, o, 0);
   ASSERT_EQ(SDB_OK, rc);
   std::thread writer = std::move(std::thread(active_writer, &rlog));
   rlog.waitUntilWriterAttached();
   constexpr UINT32 THREAD_NUM = 4;
   std::thread threads[THREAD_NUM];
   for (UINT32 i = 0; i < THREAD_NUM; ++i)
   {
      threads[i] = std::move(std::thread(thread_write, &rlog, 100000, nullptr, nullptr));
   }

   for (UINT32 i = 0; i < THREAD_NUM; ++i)
   {
      threads[i].join();
   }

   rc = rlog.flush(DPS_INVALID_LSN_OFFSET, FALSE);
   ASSERT_EQ(SDB_OK, rc);
   rlog.fini();
   writer.join();
}