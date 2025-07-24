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

   
*******************************************************************************/
#include "../testDef.h"
#include "dmsSuConstraintMap.hpp"
#include "utilSharedPtrMaker.hpp"
#include <gtest/gtest.h>

namespace engine
{
   constexpr CHAR SAMPLE_CS_1_NAME[] = "cs1";
   constexpr CHAR SAMPLE_CS_1_NEW_NAME[] = "cs1_new";
   constexpr CHAR SAMPLE_CS_1_UNIQUE_ID = 10;
   constexpr DMS_ENGINE_TYPE SAMPLE_CS_1_TYPE = DMS_ENGINE_MMAP;
   const DMS_SU_DESCRIPTOR SAMPLE_CS_1_DESC =
      makeSharedPtrFromPool< dmsSuDescriptor >( SAMPLE_CS_1_TYPE,
                                                SAMPLE_CS_1_NAME,
                                                SAMPLE_CS_1_UNIQUE_ID,
                                                1 );

   constexpr CHAR SAMPLE_CS_2_NAME[] = "cs2";
   constexpr CHAR SAMPLE_CS_2_UNIQUE_ID = 20;
   constexpr DMS_ENGINE_TYPE SAMPLE_CS_2_TYPE = DMS_ENGINE_VESSEL;
   const DMS_SU_DESCRIPTOR SAMPLE_CS_2_DESC =
      makeSharedPtrFromPool< dmsSuDescriptor >( SAMPLE_CS_2_TYPE,
                                                SAMPLE_CS_2_NAME,
                                                SAMPLE_CS_2_UNIQUE_ID,
                                                2 );

   class dms_su_constraint_map : public testing::Test
   {
   public:
      static void SetUpTestCase()
      {
         sdbEnablePD( "/opt/diaglog/sdb.log", 1, 1000 );
         setPDLevel( PDDEBUG );
      }

      static void TearDownTestCase() {}

      virtual void SetUp() override
      {
         cm.clear();
      }

      virtual void TearDown() override {}

      dmsSuConstraintMap cm;
   };

   TEST_F( dms_su_constraint_map, base_get_add )
   {
      INT32 rc = SDB_OK;
      {
         DMS_SU_DESCRIPTOR desc = nullptr;
         desc = cm.getSuDescriptor( SAMPLE_CS_1_NAME );
         EXPECT_EQ( nullptr, desc.get() );
         rc = cm.addSuDescriptor( SAMPLE_CS_1_DESC );
         ASSERT_EQ( SDB_OK, rc );
         desc = cm.getSuDescriptor( SAMPLE_CS_1_NAME );
         EXPECT_EQ( SAMPLE_CS_1_DESC, desc );
      }
      {
         DMS_SU_DESCRIPTOR desc = nullptr;
         desc = cm.getSuDescriptor( SAMPLE_CS_2_NAME );
         EXPECT_EQ( nullptr, desc.get() );
         rc = cm.addSuDescriptor( SAMPLE_CS_2_DESC );
         ASSERT_EQ( SDB_OK, rc );
         desc = cm.getSuDescriptor( SAMPLE_CS_2_NAME );
         EXPECT_EQ( SAMPLE_CS_2_DESC, desc );
      }
   }

   TEST_F( dms_su_constraint_map, base_remove )
   {
      INT32 rc = SDB_OK;
      {
         DMS_SU_DESCRIPTOR desc = nullptr;
         rc = cm.addSuDescriptor( SAMPLE_CS_1_DESC );
         ASSERT_EQ( SDB_OK, rc );
         desc = cm.getSuDescriptor( SAMPLE_CS_1_NAME );
         EXPECT_EQ( SAMPLE_CS_1_DESC, desc );
         cm.removeSuDescriptor( SAMPLE_CS_1_NAME );
         desc = cm.getSuDescriptor( SAMPLE_CS_1_NAME );
         EXPECT_EQ( nullptr, desc.get() );
      }

      {
         DMS_SU_DESCRIPTOR desc = nullptr;
         rc = cm.addSuDescriptor( SAMPLE_CS_2_DESC );
         ASSERT_EQ( SDB_OK, rc );
         desc = cm.getSuDescriptor( SAMPLE_CS_2_NAME );
         EXPECT_EQ( SAMPLE_CS_2_DESC, desc );
         cm.removeSuDescriptor( SAMPLE_CS_2_NAME );
         desc = cm.getSuDescriptor( SAMPLE_CS_2_NAME );
         EXPECT_EQ( nullptr, desc.get() );
      }
   }

   TEST_F( dms_su_constraint_map, base_prepare_create_1 )
   {
      INT32 rc = SDB_OK;
      dmsSuConstraintMap::CONTEXT_CREATE ctx;
      rc = cm.prepareToCreate( SAMPLE_CS_1_NAME, SAMPLE_CS_1_UNIQUE_ID, ctx );
      ASSERT_EQ( SDB_OK, rc );
      ctx->commit( SAMPLE_CS_1_DESC );
      DMS_SU_DESCRIPTOR desc = cm.getSuDescriptor( SAMPLE_CS_1_NAME );
      EXPECT_EQ( SAMPLE_CS_1_DESC, desc );
   }

   TEST_F( dms_su_constraint_map, base_prepare_create_2 )
   {
      INT32 rc = SDB_OK;
      dmsSuConstraintMap::CONTEXT_CREATE ctx;
      rc = cm.addSuDescriptor( SAMPLE_CS_1_DESC );
      ASSERT_EQ( SDB_OK, rc );
      rc = cm.prepareToCreate( SAMPLE_CS_1_NAME, SAMPLE_CS_1_UNIQUE_ID, ctx );
      ASSERT_EQ( SDB_DMS_CS_EXIST, rc );
   }

   TEST_F( dms_su_constraint_map, base_prepare_drop_1 )
   {
      INT32 rc = SDB_OK;
      dmsSuConstraintMap::CONTEXT_DROP ctx;
      rc = cm.prepareToDrop( SAMPLE_CS_1_NAME, ctx );
      ASSERT_EQ( SDB_DMS_CS_NOTEXIST, rc );
   }

   TEST_F( dms_su_constraint_map, base_prepare_drop_2 )
   {
      INT32 rc = SDB_OK;
      rc = cm.addSuDescriptor( SAMPLE_CS_1_DESC );
      ASSERT_EQ( SDB_OK, rc );
      dmsSuConstraintMap::CONTEXT_DROP ctx;
      rc = cm.prepareToDrop( SAMPLE_CS_1_NAME, ctx );
      ASSERT_EQ( SDB_OK, rc );
      ctx->commit();
      DMS_SU_DESCRIPTOR desc = cm.getSuDescriptor( SAMPLE_CS_1_NAME );
      EXPECT_EQ( nullptr, desc.get() );
   }

   TEST_F( dms_su_constraint_map, base_prepare_rename )
   {
      INT32 rc = SDB_OK;
      rc = cm.addSuDescriptor( SAMPLE_CS_1_DESC );
      ASSERT_EQ( SDB_OK, rc );
      dmsSuConstraintMap::CONTEXT_RENAME ctx;
      rc = cm.prepareToRename( SAMPLE_CS_1_NAME, SAMPLE_CS_1_NEW_NAME, ctx );
      ASSERT_EQ( SDB_OK, rc );
      DMS_SU_DESCRIPTOR newDesc = makeSharedPtrFromPool< dmsSuDescriptor >(
         SAMPLE_CS_1_TYPE, SAMPLE_CS_1_NEW_NAME, SAMPLE_CS_1_UNIQUE_ID, 1 );
      ctx->commit( newDesc );
      DMS_SU_DESCRIPTOR desc = cm.getSuDescriptor( SAMPLE_CS_1_NEW_NAME);
      EXPECT_EQ( newDesc.get(), desc.get() );
   }
} // namespace engine
