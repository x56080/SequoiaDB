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

   Source File Name = lpid_hash_table_test.cpp

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
#include "vessel/lpageHashTable.h"
#include <gtest/gtest.h>
#include <thread>

class lpid_hash_table_test : public testing::Test
{

};

void insert(engine::vessel::lpageHashTable *ht, INT32 factor, INT32 count)
{
   engine::vessel::lpageDescriptor v;
   for (INT32 i = 0; i < count; ++i)
   {
      engine::vessel::PAGE_ID lpid = ((factor << 24) | i);
      INT32 rc = ht->set(lpid, v);
      ASSERT_EQ(SDB_OK, rc);
   }
}

TEST_F(lpid_hash_table_test, test1)
{
   engine::vessel::lpageHashTable ht;
   constexpr INT32 TCOUNT = 8;
   INT32 count = 125000 * 2;
   std::thread threads[TCOUNT];
   for (INT32 i = 0; i < TCOUNT; ++i)
   {
      threads[i] = std::move(std::thread(insert, &ht, i, count));
   }

   for (INT32 i = 0; i < TCOUNT; ++i)
   {
      threads[i].join();
   }

   ASSERT_EQ(count * TCOUNT, ht.peek());
}