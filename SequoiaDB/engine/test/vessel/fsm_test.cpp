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

class fsm_ddl_test : public testing::Test
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

TEST_F(fsm_ddl_test, test1)
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
   rc = fsm.findInWholeMap(0, 1233, FALSE, candidate);
   ASSERT_EQ(SDB_VESSEL_FSM_NO_FREE_SPACE, rc);

   PAGE_ID lpids[8];
   for (UINT32 i = 0; i < 8; ++i)
   {
      lpids[i] = i;
   }

   rc = fsm.addNewPages(0, lpids, 8);
   ASSERT_EQ(SDB_OK, rc);

   rc = fsm.findInWholeMap(0, 1233, TRUE, candidate);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(candidate.seq, 0);
   ASSERT_EQ(candidate.lpid, 0);
   ASSERT_EQ(candidate.free, 32676);

   fsm.close();
   file.close();
}