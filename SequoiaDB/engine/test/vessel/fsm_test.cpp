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

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

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

TEST_F(fsm_test, test1)
{
   INT32 rc = SDB_OK;
   v::freeSpaceMap fsm;
   v::fsmFile file;
   v::storageCoreArgs args;
   args.pageSize = DMS_PAGE_SIZE32K;
   args.maxPageCountPerSeg = FSM_PAGE_COUNT_PER_SEG;
   args.maxSegmentCountPerFile = FSM_MAX_SEG_COUNT;
   v::storageFileOptions options;
   options.args = &args;
   options.dir = DATA_PATH;
   options.name = "_space_0.fsm";
   options.secretValue = 0;
   options.spaceID = 0;
   options.sequence = 0;
   rc = file.create(options);
   ASSERT_EQ(SDB_OK, rc);
   rc = file.initAfterCreation();
   ASSERT_EQ(SDB_OK, rc);
   v::fsmCandidate candidate;

   rc = fsm.create(&file, 0, 0, DMS_PAGE_SIZE32K, 0, TRUE, 0, 4095);
   ASSERT_EQ(SDB_OK, rc);

   rc = fsm.fastFind(0, 1233, candidate);
   ASSERT_EQ(SDB_VESSEL_FSM_NO_FREE_SPACE, rc);
   rc = fsm.findInWholeMap(0, 1233, 0, candidate);
   ASSERT_EQ(SDB_VESSEL_FSM_NO_FREE_SPACE, rc);

   PAGE_ID lpids[8];
   for (UINT32 i = 0; i < 8; ++i)
   {
      lpids[i] = i;
   }

   rc = fsm.addNewPages(0, lpids, 8);
   ASSERT_EQ(SDB_OK, rc);

   rc = fsm.findInWholeMap(0, 1233, 0, candidate);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(candidate.seq, 0);
   ASSERT_EQ(candidate.lpid, 0);
   ASSERT_EQ(candidate.free, 31420);

   fsm.close();
   file.close();

   v::storageFileName fn;
   fn.build(SPACE_TYPE_FSM, 0, 0);
   rc = file.open(std::string(DATA_PATH).append("/_space_0.fsm").c_str(), fn);
   ASSERT_EQ(SDB_OK, rc);

   rc = fsm.open(&file, 0, 0, DMS_PAGE_SIZE32K, 0, TRUE, 0, 4095);
   ASSERT_EQ(SDB_OK, rc);

   candidate.reset();
   rc = fsm.fastFind(0, 1223, candidate);
   ASSERT_EQ(SDB_VESSEL_FSM_NO_FREE_SPACE, rc);
   rc = fsm.findInWholeMap(0, 1233, 0, candidate);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(candidate.seq, 0);
   ASSERT_EQ(candidate.lpid, INVALID_PAGE_ID);
   ASSERT_EQ(candidate.free, 29976);

   fsm.close();
   file.close();
}

TEST_F(fsm_test, diskmap_0)
{
   INT32 rc = SDB_OK;
   v::fsmFile file;
   v::storageCoreArgs args;
   args.pageSize = DMS_PAGE_SIZE32K;
   args.maxPageCountPerSeg = FSM_PAGE_COUNT_PER_SEG;
   args.maxSegmentCountPerFile = FSM_MAX_SEG_COUNT;
   v::storageFileOptions options;
   options.args = &args;
   options.dir = DATA_PATH;
   options.name = "_space_0.fsm";
   options.secretValue = 0;
   options.spaceID = 0;
   options.sequence = 0;
   rc = file.create(options);
   ASSERT_EQ(SDB_OK, rc);
   rc = file.initAfterCreation();
   ASSERT_EQ(SDB_OK, rc);
   v::diskFreeSpaceMap diskMap;
   rc = diskMap.create(&file, 0, 0);
   ASSERT_EQ(SDB_OK, rc);
   UINT32 count = FSM_SEQ_RANGE_IN_PAGE * 4;
   v::fsmSizeLvl lvl;
   lvl.init(32768, 32768);

   for (UINT32 i = 0; i < count; ++i)
   {
      rc = diskMap.addNewPages(i * 8, 8);
      ASSERT_EQ(SDB_OK, rc);
   }
   
   for (UINT32 i = 0; i < count; ++i)
   {
      for (UINT32 j = 0; j < 8; ++j)
      {
         rc = diskMap.updatePageFreeSizeLvL(i * 8 + j, lvl);
         ASSERT_EQ(SDB_OK, rc);
      }
   }

   diskMap.close();

   rc = diskMap.open(&file, 0, 0);
   ASSERT_EQ(SDB_OK, rc);
   CL_PAGE_SEQ candidate = INVALID_CL_PAGE_SEQ;
   fsmSizeLvl candidateLvl;
   for (UINT32 i = 0; i < count; ++i)
   {
      for (UINT32 j = 0; j < 8; ++j)
      {
         candidate = INVALID_CL_PAGE_SEQ;
         candidateLvl.reset();
         rc = diskMap.find(lvl, candidate, candidateLvl, 0.0);
         ASSERT_EQ(SDB_OK, rc);
         ASSERT_EQ(i * 8 + j, candidate);
         ASSERT_EQ(FSM_SPACE_LVL3, candidateLvl.getLvl());
         ASSERT_EQ(15, candidateLvl.getDelta());
      }
   }

   candidate = INVALID_CL_PAGE_SEQ;
   candidateLvl.reset();
   rc = diskMap.find(lvl, candidate, candidateLvl, 0.0);
   ASSERT_EQ(SDB_VESSEL_FSM_NO_FREE_SPACE, rc);
   diskMap.close();
   file.close();
   
}

TEST_F(fsm_test, diskmap_1)
{
   INT32 rc = SDB_OK;
   v::fsmFile file;
   v::storageCoreArgs args;
   args.pageSize = DMS_PAGE_SIZE32K;
   args.maxPageCountPerSeg = FSM_PAGE_COUNT_PER_SEG;
   args.maxSegmentCountPerFile = FSM_MAX_SEG_COUNT;
   v::storageFileOptions options;
   options.args = &args;
   options.dir = DATA_PATH;
   options.name = "_space_0.fsm";
   options.secretValue = 0;
   options.spaceID = 0;
   options.sequence = 0;
   rc = file.create(options);
   ASSERT_EQ(SDB_OK, rc);
   rc = file.initAfterCreation();
   ASSERT_EQ(SDB_OK, rc);
   v::diskFreeSpaceMap diskMap;
   rc = diskMap.create(&file, 0, 0);
   ASSERT_EQ(SDB_OK, rc);
   UINT32 count = FSM_SEQ_RANGE_IN_PAGE * 4;
   v::fsmSizeLvl lvl0;
   v::fsmSizeLvl lvl1;
   lvl0.init(32768, 32768);
   lvl1.init(32768, 25087);/// lvl3 delta 0

   for (UINT32 i = 0; i < count; ++i)
   {
      rc = diskMap.addNewPages(i * 8, 8);
      ASSERT_EQ(SDB_OK, rc);
   }
   
   for (UINT32 i = 0; i < count; ++i)
   {
      for (UINT32 j = 0; j < 8; ++j)
      {
         if (0 == (j & 0x01))
         {
            rc = diskMap.updatePageFreeSizeLvL(i * 8 + j, lvl0);
            ASSERT_EQ(SDB_OK, rc);
         }
         else
         {
            rc = diskMap.updatePageFreeSizeLvL(i * 8 + j, lvl1);
            ASSERT_EQ(SDB_OK, rc);
         }
      }
   }

   diskMap.close();

   rc = diskMap.open(&file, 0, 0);
   ASSERT_EQ(SDB_OK, rc);

   CL_PAGE_SEQ candidate = INVALID_CL_PAGE_SEQ;
   fsmSizeLvl candidateLvl;

   for (UINT32 i = 0; i < count * 8; i = i + 2)
   {
      candidate = INVALID_CL_PAGE_SEQ;
      candidateLvl.reset();
      rc = diskMap.find(lvl0, candidate, candidateLvl, 0.0);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(i, candidate);
      ASSERT_EQ(FSM_SPACE_LVL3, candidateLvl.getLvl());
      ASSERT_EQ(15, candidateLvl.getDelta());
   }
   rc = diskMap.find(lvl0, candidate, candidateLvl, 0.0);
   ASSERT_EQ(SDB_VESSEL_FSM_NO_FREE_SPACE, rc);

   for (UINT32 i = 1; i < count * 8; i = i + 2)
   {
      candidate = INVALID_CL_PAGE_SEQ;
      candidateLvl.reset();
      rc = diskMap.find(lvl1, candidate, candidateLvl, 0.0);
      ASSERT_EQ(SDB_OK, rc);
      //ASSERT_EQ(i, candidate);   cursor moved by last scan loop.
      ASSERT_EQ(FSM_SPACE_LVL3, candidateLvl.getLvl());
      ASSERT_EQ(0, candidateLvl.getDelta());
   }
   rc = diskMap.find(lvl1, candidate, candidateLvl, 0.0);
   ASSERT_EQ(SDB_VESSEL_FSM_NO_FREE_SPACE, rc);

   diskMap.close();
   file.close();
   
}

TEST_F(fsm_test, diskmap_2)
{
   INT32 rc = SDB_OK;
   v::fsmFile file;
   v::storageCoreArgs args;
   args.pageSize = DMS_PAGE_SIZE32K;
   args.maxPageCountPerSeg = FSM_PAGE_COUNT_PER_SEG;
   args.maxSegmentCountPerFile = FSM_MAX_SEG_COUNT;
   v::storageFileOptions options;
   options.args = &args;
   options.dir = DATA_PATH;
   options.name = "_space_0.fsm";
   options.secretValue = 0;
   options.spaceID = 0;
   options.sequence = 0;
   rc = file.create(options);
   ASSERT_EQ(SDB_OK, rc);
   rc = file.initAfterCreation();
   ASSERT_EQ(SDB_OK, rc);
   v::diskFreeSpaceMap diskMap;
   rc = diskMap.create(&file, 0, 0);
   ASSERT_EQ(SDB_OK, rc);
   UINT32 count = FSM_SEQ_RANGE_IN_PAGE * 80; /// about 10 - 11 segment
   v::fsmSizeLvl lvl;
   lvl.init(32768, 32768);

   CL_PAGE_SEQ candidate = INVALID_CL_PAGE_SEQ;
   fsmSizeLvl candidateLvl;

   for (UINT32 i = 0; i < count; ++i)
   {
      rc = diskMap.addNewPages(i * 8, 8);
      ASSERT_EQ(SDB_OK, rc);
   }
   
   for (UINT32 i = 0; i < count; ++i)
   {
      for (UINT32 j = 0; j < 8; ++j)
      {
         rc = diskMap.updatePageFreeSizeLvL(i * 8 + j, lvl);
         ASSERT_EQ(SDB_OK, rc);
      }
   }

   for (UINT32 i = 0; i < count; ++i)
   {
      for (UINT32 j = 0; j < 8; ++j)
      {
         candidate = INVALID_CL_PAGE_SEQ;
         candidateLvl.reset();
         rc = diskMap.find(lvl, candidate, candidateLvl, 0.0);
         ASSERT_EQ(SDB_OK, rc);
         ASSERT_EQ(i * 8 + j, candidate);
         ASSERT_EQ(FSM_SPACE_LVL3, candidateLvl.getLvl());
         ASSERT_EQ(15, candidateLvl.getDelta());
      }
   }

   candidate = INVALID_CL_PAGE_SEQ;
   candidateLvl.reset();
   rc = diskMap.find(lvl, candidate, candidateLvl, 0.0);
   ASSERT_EQ(SDB_VESSEL_FSM_NO_FREE_SPACE, rc);

   for (UINT32 i = 0; i < count; ++i)
   {
      for (UINT32 j = 0; j < 8; ++j)
      {
         rc = diskMap.updatePageFreeSizeLvL(i * 8 + j, lvl);
         ASSERT_EQ(SDB_OK, rc);
      }
   }

   diskMap.close();

   rc = diskMap.open(&file, 0, 0);
   ASSERT_EQ(SDB_OK, rc);
   
   for (UINT32 i = 0; i < count; ++i)
   {
      for (UINT32 j = 0; j < 8; ++j)
      {
         candidate = INVALID_CL_PAGE_SEQ;
         candidateLvl.reset();
         rc = diskMap.find(lvl, candidate, candidateLvl, 0.0);
         ASSERT_EQ(SDB_OK, rc);
         ASSERT_EQ(i * 8 + j, candidate);
         ASSERT_EQ(FSM_SPACE_LVL3, candidateLvl.getLvl());
         ASSERT_EQ(15, candidateLvl.getDelta());
      }
   }

   candidate = INVALID_CL_PAGE_SEQ;
   candidateLvl.reset();
   rc = diskMap.find(lvl, candidate, candidateLvl, 0.0);
   ASSERT_EQ(SDB_VESSEL_FSM_NO_FREE_SPACE, rc);
   diskMap.close();
   file.close();
   
}

TEST_F(fsm_test, diskmap_3)
{
   INT32 rc = SDB_OK;
   v::fsmFile file;
   v::storageCoreArgs args;
   args.pageSize = DMS_PAGE_SIZE32K;
   args.maxPageCountPerSeg = FSM_PAGE_COUNT_PER_SEG;
   args.maxSegmentCountPerFile = FSM_MAX_SEG_COUNT;
   v::storageFileOptions options;
   options.args = &args;
   options.dir = DATA_PATH;
   options.name = "_space_0.fsm";
   options.secretValue = 0;
   options.spaceID = 0;
   options.sequence = 0;
   rc = file.create(options);
   ASSERT_EQ(SDB_OK, rc);
   rc = file.initAfterCreation();
   ASSERT_EQ(SDB_OK, rc);
   UINT32 count = 65535;
   storageFileName fn;
   fn.build(SPACE_TYPE_FSM, 0, 0);
   std::string fullPath;
   fullPath.append(DATA_PATH);
   fullPath.append(OSS_FILE_SEP);
   fullPath.append(fn.getName());

   for (UINT32 i = 0; i < count; ++i)
   {
      v::diskFreeSpaceMap diskMap;
      rc = diskMap.create(&file, i, i);
      ASSERT_EQ(SDB_OK, rc);
      diskMap.close();
   }
   file.close();

   rc = file.open(fullPath.c_str(), fn);
   ASSERT_EQ(SDB_OK, rc);
   rc = file.initAfterOpen();
   ASSERT_EQ(SDB_OK, rc);
   for (UINT32 i = 0; i < count; ++i)
   {
      v::diskFreeSpaceMap diskMap;
      rc = diskMap.open(&file, i, i, FALSE);
      ASSERT_EQ(SDB_OK, rc);
      diskMap.close();
   }
   file.close();
}