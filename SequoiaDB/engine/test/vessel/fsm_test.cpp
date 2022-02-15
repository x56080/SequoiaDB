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

   Source File Name = fsm_test.cpp

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
#include "vessel/freeSpaceMap.h"
#include "vessel/recordDataPage.h"

#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
namespace v = engine::vessel;

class fsm_test : public testing::Test
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


TEST_F(fsm_test, diskmap_0)
{
   INT32 rc = SDB_OK;
   v::fsmFile file;
   v::createStorageFileOptions options;
   options.args.pageSize = FSM_FILE_PAGE_SIZE;
   options.args.maxPageCountPerSeg = FSM_FILE_PAGE_COUNT_PER_SEG;
   options.args.maxSegmentCountPerFile = FSM_FILE_MAX_SEG_COUNT;
   v::storageFileName fn;
   UINT32 count = 8;

   strSlice dirSlice(DATA_PATH);

   v::diskFreeSpaceMap dfsm;

   ASSERT_TRUE(fn.build(FILE_TYPE_FSM, SPACE_TYPE_MAIN_DATA));
   rc = file.create(dirSlice, fn, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = file.initToWork();
   ASSERT_EQ(SDB_OK, rc);
   
   rc = dfsm.create(&file, 0, 0);
   ASSERT_EQ(SDB_OK, rc);

   INT32 targetLvl = FSM_MIN_SPACE_LVL;
   INT32 lvl = FSM_INVALID_SPACE_LVL;
   UINT32 seq = INVALID_CL_PAGE_SEQ;
   BOOLEAN found = FALSE;
   rc = dfsm.find(targetLvl, found, seq, lvl);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_FALSE(found);

   rc = dfsm.incDataPageCount(count);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      rc = dfsm.upgradePageSpaceLvl(i, FSM_MAX_SPACE_LVL);
      ASSERT_EQ(SDB_OK, rc);
   }

   for (UINT32 i = 0; i < count; ++i)
   {
      rc = dfsm.find(targetLvl, found, seq, lvl);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_TRUE(found);
      ASSERT_EQ(i, seq);
      ASSERT_EQ(FSM_MAX_SPACE_LVL, lvl);
   }

   rc = dfsm.find(targetLvl, found, seq, lvl);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_FALSE(found);

   dfsm.close();
   file.close();
}

TEST_F(fsm_test, diskmap_1)
{
   INT32 rc = SDB_OK;
   v::fsmFile file;
   v::createStorageFileOptions options;
   options.args.pageSize = FSM_FILE_PAGE_SIZE;
   options.args.maxPageCountPerSeg = FSM_FILE_PAGE_COUNT_PER_SEG;
   options.args.maxSegmentCountPerFile = FSM_FILE_MAX_SEG_COUNT;
   v::storageFileName fn;
   UINT32 count = 65536 * 10 + 1;
   strSlice dirSlice(DATA_PATH);

   v::diskFreeSpaceMap dfsm;

   ASSERT_TRUE(fn.build(FILE_TYPE_FSM, SPACE_TYPE_MAIN_DATA));
   rc = file.create(dirSlice, fn, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = file.initToWork();
   ASSERT_EQ(SDB_OK, rc);
   
   rc = dfsm.create(&file, 0, 0);
   ASSERT_EQ(SDB_OK, rc);

   INT32 targetLvl = FSM_MIN_SPACE_LVL;
   INT32 lvl = FSM_INVALID_SPACE_LVL;
   UINT32 seq = INVALID_CL_PAGE_SEQ;
   BOOLEAN found = FALSE;
   rc = dfsm.find(targetLvl, found, seq, lvl);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_FALSE(found);

   rc = dfsm.incDataPageCount(count);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      rc = dfsm.upgradePageSpaceLvl(i, FSM_MAX_SPACE_LVL);
      ASSERT_EQ(SDB_OK, rc);
   }

   for (UINT32 i = 0; i < count; ++i)
   {
      rc = dfsm.find(targetLvl, found, seq, lvl);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_TRUE(found);
      ASSERT_EQ(i, seq);
      ASSERT_EQ(FSM_MAX_SPACE_LVL, lvl);
   }

   rc = dfsm.find(targetLvl, found, seq, lvl);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_FALSE(found);

   dfsm.close();
   file.close();
}

TEST_F(fsm_test, diskmap_2)
{
   INT32 rc = SDB_OK;
   v::fsmFile file;
   v::createStorageFileOptions options;

   options.args.pageSize = FSM_FILE_PAGE_SIZE;
   options.args.maxPageCountPerSeg = FSM_FILE_PAGE_COUNT_PER_SEG;
   options.args.maxSegmentCountPerFile = FSM_FILE_MAX_SEG_COUNT;
   v::storageFileName fn;
   UINT32 count = 65536 * 100 + 1;
   strSlice dirSlice(DATA_PATH);
   v::diskFreeSpaceMap dfsm;

   ASSERT_TRUE(fn.build(FILE_TYPE_FSM, SPACE_TYPE_MAIN_DATA));
   rc = file.create(dirSlice, fn, options);
   ASSERT_EQ(SDB_OK, rc);

   rc = file.initToWork();
   ASSERT_EQ(SDB_OK, rc);
   
   rc = dfsm.create(&file, 0, 0);
   ASSERT_EQ(SDB_OK, rc);

   INT32 targetLvl = FSM_MIN_SPACE_LVL;
   INT32 lvl = FSM_INVALID_SPACE_LVL;
   UINT32 seq = INVALID_CL_PAGE_SEQ;
   BOOLEAN found = FALSE;
   rc = dfsm.find(targetLvl, found, seq, lvl);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_FALSE(found);

   rc = dfsm.incDataPageCount(count);
   ASSERT_EQ(SDB_OK, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      rc = dfsm.upgradePageSpaceLvl(i, FSM_MAX_SPACE_LVL);
      ASSERT_EQ(SDB_OK, rc);
   }

   for (UINT32 i = 0; i < count; ++i)
   {
      rc = dfsm.find(targetLvl, found, seq, lvl);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_TRUE(found);
      ASSERT_EQ(i, seq);
      ASSERT_EQ(FSM_MAX_SPACE_LVL, lvl);
   }

   rc = dfsm.find(targetLvl, found, seq, lvl);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_FALSE(found);

   dfsm.close();
   file.close();
}
