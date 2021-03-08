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

   Source File Name = collectionmap_test.cpp

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

#include "vessel/collectionAllocator.h"
#include <gtest/gtest.h>

using namespace engine::vessel;

TEST(collectionallocator, test1)
{
   collectionAllocator allocator;
   collectionAllocator::collectionHolder *holder = NULL;
   CL_MB_ID mbID = INVALID_CL_MB_ID;
   INT32 rc = SDB_OK;
   for (UINT32 i = 0; i < 1000; ++i)
   {
      rc = allocator.allocateNewMB(mbID, &holder);
      ASSERT_EQ(SDB_OK, rc);
      ASSERT_EQ(i, mbID);
   }
   allocator.fini();
}

TEST(collectionallocator, test2)
{
   collectionAllocator allocator;
   collectionAllocator::collectionHolder *holder = NULL;
   CL_MB_ID mbID = INVALID_CL_MB_ID;
   INT32 rc = SDB_OK;
   for (UINT32 i = 0; i < 1000; ++i)
   {
      rc = allocator.occupyMB(i, NULL);
      ASSERT_EQ(SDB_OK, rc);
   }

   rc = allocator.allocateNewMB(mbID, NULL);
   ASSERT_EQ(SDB_OK, rc);
   ASSERT_EQ(1000, mbID);
   allocator.fini();
}