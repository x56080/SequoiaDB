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

   Source File Name = lpid_hash_table_test.cpp

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