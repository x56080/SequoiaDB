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

   Source File Name = dpsReqBuilderTest.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dpsWriteReqBuilder.hpp"
#include "dpsLogRecordDef.hpp"
#include <gtest/gtest.h>

using namespace engine;

class dpsReqBuilderTest :  public testing::Test
{
   public:
   static void SetUpTestCase(){}
   static void TearDownTestCase(){}
   virtual void SetUp(){}
};

TEST_F(dpsReqBuilderTest, appendAndCheckElements)
{
   INT32 rc = SDB_OK;
   dpsWriteReqBuilder builder;
   dpsWriteRequest req;

   DPS_LOG_TYPE type = LOG_TYPE_CS_CRT;
   UINT16 flags = 0x01;
   std::string name = "foo";
   UINT32 unique = 1;
   UINT32 size = 64 * 1024;

   builder.setType(type);
   builder.setFlag(flags);
   rc = builder.append(DPS_LOG_CSCRT_CSNAME, name.size(), name.c_str());
   ASSERT_EQ(SDB_OK, rc);
   rc = builder.appendNumeric(DPS_LOG_CSCRT_PAGESIZE, size);
   ASSERT_EQ(SDB_OK, rc);
   rc = builder.appendInt32(DPS_LOG_CSCRT_CSUNIQUEID, unique);
   ASSERT_EQ(SDB_OK, rc);

   req = builder.reap();
   ASSERT_EQ(req.getFlags(), flags);
   ASSERT_EQ(req.getType(), type);
   ASSERT_EQ(req.getElements().getElementNum(), (UINT32)3);

   utilSlice slice;
   ASSERT_TRUE(req.seek(DPS_LOG_CSCRT_CSNAME, slice));
   ASSERT_TRUE(slice.isValid());
   ASSERT_EQ(0, ossMemcmp(name.c_str(), slice.data(), slice.getSize()));

   ASSERT_FALSE(req.seek(DPS_LOG_CSCRT_VESSEL_SID, slice));
   ASSERT_FALSE(slice.isValid());

   ASSERT_TRUE(req.seek(DPS_LOG_CSCRT_PAGESIZE, slice));
   ASSERT_TRUE(slice.isValid());
   const UINT32 *resSize = slice.castTo<UINT32>();
   ASSERT_EQ(*resSize, size);

   ASSERT_TRUE(req.seek(DPS_LOG_CSCRT_CSUNIQUEID, slice));
   ASSERT_TRUE(slice.isValid());
   resSize = slice.castTo<UINT32>();
   ASSERT_EQ(*resSize, unique);

   req.reset();
   ASSERT_FALSE(req.seek(DPS_LOG_CSCRT_CSNAME, slice));
   ASSERT_FALSE(slice.isValid());
}

TEST_F(dpsReqBuilderTest, elementsItr)
{
   INT32 rc = SDB_OK;
   dpsWriteReqBuilder builder;
   dpsWriteRequest req;

   rc = builder.appendInt32(1, 1);
   ASSERT_EQ(SDB_OK, rc);
   rc = builder.appendInt32(2, 2);
   ASSERT_EQ(SDB_OK, rc);
   rc = builder.appendInt32(3, 3);
   ASSERT_EQ(SDB_OK, rc);

   req = builder.reap();
   const dpsRecordElements &eles = req.getElements();
   UINT32 i = 1;
   auto itr = eles.begin();
   while (itr.isValid())
   {
      ASSERT_EQ(itr.getTag(), i);
      ASSERT_EQ(*(UINT32 *)(itr.getValue().getData()), i);
      ++i;
      itr.next();
   }
}

TEST_F(dpsReqBuilderTest, trivial)
{
   INT32 rc = SDB_OK;
   dpsWriteReqBuilder builder;
   dpsWriteRequest req;
   constexpr UINT32 padSize = 4096;
   UINT32 eleCount = 10;
   CHAR pad[4096] = {};
   ossMemset(pad, 'a', sizeof(pad)); 
   builder.append(1, padSize, pad);

   dpsTrivialElement te = builder.startToBuildTsElement(2);
   for (UINT32 i = 0; i < eleCount; ++i)
   {
      rc = te.appendNumeric(i, i);
      ASSERT_EQ(SDB_OK, rc);
   }
   te.done();

   utilSlice slice;
   req = builder.reap();
   ASSERT_TRUE(req.seek(1, slice));
   ASSERT_EQ(0, ossMemcmp(pad, slice.getData(), padSize));
   
   ASSERT_TRUE(req.seek(2, slice));
   dpsTrivialString ts(slice.getData(), slice.getSize());
   auto itr = ts.begin();
   UINT32 i = 0;
   while (itr.isValid())
   {
      ASSERT_TRUE(itr.getField().isValid());
      ASSERT_EQ(itr.getField().getTag(), i);
      ASSERT_EQ(itr.getField().getNumericValue<UINT32>(), i);
      ASSERT_EQ(itr.getField().getValueSize(), sizeof(UINT32));
      itr.next();
      ++i;
   }
}