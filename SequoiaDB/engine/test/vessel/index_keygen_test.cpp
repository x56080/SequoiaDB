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

   Source File Name = index_keygen_test.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/indexKeyGenerator.h"
#include <gtest/gtest.h>
#include "vessel/slice.h"



TEST(index_key_gen, test1)
{
   bson::BSONObjBuilder builder;
   builder.append("a", 1);
   bson::BSONObj pattern = builder.obj();

   /// reuse as record
   ::engine::vessel::slice record(pattern.objsize(), pattern.objdata());
   bson::BSONObjSet keys;

   for (UINT32 i = 0; i < 1000000; ++i)
   {
      keys.clear();
      INT32 rc = ::engine::vessel::indexKeyGenForBsonRecord(pattern, FALSE, record, keys);
      ASSERT_EQ(SDB_OK, rc);
   }
}