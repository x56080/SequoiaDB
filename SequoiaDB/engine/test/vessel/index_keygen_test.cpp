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

   Source File Name = index_keygen_test.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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