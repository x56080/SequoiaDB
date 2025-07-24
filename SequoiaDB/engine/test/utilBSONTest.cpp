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

   Source File Name = utilBSONTest.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilBSON.hpp"

#include "ossMemPool.hpp"

#include <../bson/bson.h>
#include "gtest/gtest.h"

#include <string>
using namespace std;
using namespace engine;
using namespace engine::util;
using namespace bson;

class utilBSONTest : public ::testing::Test
{
 protected:
   const string _field;
 public:
   utilBSONTest() : _field("TestField"){};
};

// Test case where field does not exist
TEST_F (utilBSONTest, NotFound)
{
   INT32 x;
   BSONObj obj = BSON("notField" << 123);
   ASSERT_EQ(SDB_FIELD_NOT_EXIST, fromBsonObj(obj, _field, &x));
   ASSERT_EQ(SDB_OK, fromBsonObj(obj, _field, &x, FALSE));
}

TEST_F (utilBSONTest, INT32Test)
{
   INT32 x = 123;
   INT32 y;
   BSONObj obj = BSON(_field << x);
   ASSERT_EQ(SDB_OK, fromBsonObj(obj, _field, &y));
   ASSERT_EQ(x, y);
}

TEST_F (utilBSONTest, UINT32)
{
   UINT32 x = 123;
   UINT32 y;
   BSONObj obj = BSON(_field << x);
   ASSERT_EQ(SDB_OK, fromBsonObj(obj, _field, &y));
   ASSERT_EQ(x, y);
   obj = BSON(_field << -1);
   ASSERT_EQ(SDB_INVALIDARG, fromBsonObj(obj, _field, &y));
}

TEST_F (utilBSONTest, INT64)
{
   INT64 x = 123;
   INT64 y;
   BSONObj obj = BSON(_field << x);
   ASSERT_EQ(SDB_OK, fromBsonObj(obj, _field, &y));
   ASSERT_EQ(x, y);
}

TEST_F (utilBSONTest, UINT64)
{
   UINT64 x = 123;
   UINT64 y;
   BSONObj obj = BSON(_field << (INT64)x);
   ASSERT_EQ(SDB_OK, fromBsonObj(obj, _field, &y));
   ASSERT_EQ(x, y);
   obj = BSON(_field << -1);
   ASSERT_EQ(SDB_INVALIDARG, fromBsonObj(obj, _field, &y));
}

TEST_F (utilBSONTest, BOOLEAN)
{
   BOOLEAN x;
   BSONObj obj = BSON(_field << 1);
   ASSERT_EQ(SDB_OK, boolFromBsonObj(obj, _field, &x));
   ASSERT_EQ(x, TRUE);
   obj = BSON(_field << true);
   ASSERT_EQ(SDB_OK, boolFromBsonObj(obj, _field, &x));
   ASSERT_EQ(x, TRUE);
   obj = BSON(_field << 0);
   ASSERT_EQ(SDB_OK, boolFromBsonObj(obj, _field, &x));
   ASSERT_EQ(x, FALSE);
   obj = BSON(_field << false);
   ASSERT_EQ(SDB_OK, boolFromBsonObj(obj, _field, &x));
   ASSERT_EQ(x, FALSE);
   obj = BSONObj();
   ASSERT_EQ(SDB_OK, boolFromBsonObj(obj, _field, &x));
   ASSERT_EQ(x, FALSE);
}

TEST_F (utilBSONTest, String)
{
   string x = "test";
   BSONObj obj = BSON(_field << x);
   string y;
   ASSERT_EQ(SDB_OK, fromBsonObj(obj, _field, &y));
   ASSERT_STREQ(x.c_str(), y.c_str());
   ossPoolString z;
   ASSERT_EQ(SDB_OK, fromBsonObj(obj, _field, &z));
   ASSERT_STREQ(x.c_str(), z.c_str());
}

TEST_F (utilBSONTest, SubObject)
{
   BSONObj subobj = BSON("Subobj" << 123);
   BSONObj obj = BSON(_field << subobj);
   BSONObj x;
   ASSERT_EQ(SDB_OK, fromBsonObj(obj, _field, &x));
   ASSERT_EQ(x, subobj);
}


// Test case where field is incompatible type
TEST_F (utilBSONTest, BadType)
{
   INT32 x;
   BSONObj obj = BSON(_field << "hello");
   ASSERT_EQ(SDB_INVALIDARG, fromBsonObj(obj, _field, &x));
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

