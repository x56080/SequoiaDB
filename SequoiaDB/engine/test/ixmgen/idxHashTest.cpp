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

   Source File Name = generator.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/

#include "dmsStorageDataCommon.hpp"
#include "mthModifier.hpp"
#include "../bson/bson.h"

#include "gtest/gtest.h"
#include <iostream>

using namespace std ;
using namespace engine ;
using namespace bson ;

TEST( generator, test_base )
{
   dmsMBStatInfo mbStatInfo ;
   mbStatInfo.setIdxHash( 0, "_id" ) ;
   mbStatInfo.setIdxHash( 1, "a" ) ;
   mbStatInfo.setIdxHash( 1, "b" ) ;

   cout << "cl: " << mbStatInfo._clIdxHashBitmap.toString() << endl ;
   cout << "idx 0: " << mbStatInfo._idxHashBitmaps[ 0 ].toString() << endl ;
   cout << "idx 1: " << mbStatInfo._idxHashBitmaps[ 1 ].toString() << endl ;

   ixmIdxHashBitmap fieldBitmap1 ;
   fieldBitmap1.setFieldBit( "_id" ) ;

   cout << "id : " << fieldBitmap1.toString() << endl ;

   ixmIdxHashBitmap fieldBitmap2 ;
   fieldBitmap2.setFieldBit( "a" ) ;

   cout << "a : " << fieldBitmap2.toString() << endl ;

   ixmIdxHashBitmap fieldBitmap3 ;
   fieldBitmap3.setFieldBit( "a" ) ;
   fieldBitmap3.setFieldBit( "b" ) ;

   cout << "a, b : " << fieldBitmap3.toString() << endl ;

   ixmIdxHashBitmap fieldBitmap4 ;
   fieldBitmap4.setFieldBit( "_id" ) ;
   fieldBitmap4.setFieldBit( "a" ) ;
   fieldBitmap4.setFieldBit( "b" ) ;

   cout << "id, a, b : " << fieldBitmap4.toString() << endl ;

   ixmIdxHashBitmap fieldBitmap5 ;
   fieldBitmap5.setFieldBit( "d" ) ;

   cout << "d : " << fieldBitmap5.toString() << endl ;

   ixmIdxHashBitmap fieldBitmap6 ;
   fieldBitmap6.setFieldBit( "a1" ) ;

   cout << "a1 : " << fieldBitmap6.toString() << endl ;

   ASSERT_TRUE( mbStatInfo._clIdxHashBitmap.isEqual( fieldBitmap4 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashBitmaps[ 0 ].isEqual( fieldBitmap1 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashBitmaps[ 1 ].isEqual( fieldBitmap3 ) ) ;
   for ( UINT32 i = 2 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
   {
      ASSERT_TRUE( mbStatInfo._idxHashBitmaps[ i ].isEmpty() ) ;
   }

   ASSERT_TRUE( mbStatInfo.testIdxHash( fieldBitmap1 ) ) ;
   ASSERT_TRUE( mbStatInfo.testIdxHash( 0, fieldBitmap1 ) ) ;
   for ( UINT32 i = 1 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
   {
      ASSERT_FALSE( mbStatInfo.testIdxHash( i, fieldBitmap1 ) ) ;
   }
   for ( UINT32 i = IXM_IDX_HASH_MAX_INDEX_NUM ;
         i < DMS_COLLECTION_MAX_INDEX ;
         ++ i )
   {
      ASSERT_TRUE( mbStatInfo.testIdxHash( i, fieldBitmap1 ) ) ;
   }

   ASSERT_TRUE( mbStatInfo.testIdxHash( fieldBitmap2 ) ) ;
   ASSERT_FALSE( mbStatInfo.testIdxHash( 0, fieldBitmap2 ) ) ;
   ASSERT_TRUE( mbStatInfo.testIdxHash( 1, fieldBitmap2 ) ) ;
   for ( UINT32 i = 2 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
   {
      ASSERT_FALSE( mbStatInfo.testIdxHash( i, fieldBitmap2 ) ) ;
   }
   for ( UINT32 i = IXM_IDX_HASH_MAX_INDEX_NUM ;
         i < DMS_COLLECTION_MAX_INDEX ;
         ++ i )
   {
      ASSERT_TRUE( mbStatInfo.testIdxHash( i, fieldBitmap2 ) ) ;
   }

   ASSERT_TRUE( mbStatInfo.testIdxHash( fieldBitmap3 ) ) ;
   ASSERT_FALSE( mbStatInfo.testIdxHash( 0, fieldBitmap3 ) ) ;
   ASSERT_TRUE( mbStatInfo.testIdxHash( 1, fieldBitmap3 ) ) ;
   for ( UINT32 i = 2 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
   {
      ASSERT_FALSE( mbStatInfo.testIdxHash( i, fieldBitmap3 ) ) ;
   }
   for ( UINT32 i = IXM_IDX_HASH_MAX_INDEX_NUM ;
         i < DMS_COLLECTION_MAX_INDEX ;
         ++ i )
   {
      ASSERT_TRUE( mbStatInfo.testIdxHash( i, fieldBitmap3 ) ) ;
   }

   ASSERT_TRUE( mbStatInfo.testIdxHash( fieldBitmap4 ) ) ;
   ASSERT_TRUE( mbStatInfo.testIdxHash( 0, fieldBitmap4 ) ) ;
   ASSERT_TRUE( mbStatInfo.testIdxHash( 1, fieldBitmap4 ) ) ;
   for ( UINT32 i = 2 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
   {
      ASSERT_FALSE( mbStatInfo.testIdxHash( i, fieldBitmap4 ) ) ;
   }
   for ( UINT32 i = IXM_IDX_HASH_MAX_INDEX_NUM ;
         i < DMS_COLLECTION_MAX_INDEX ;
         ++ i )
   {
      ASSERT_TRUE( mbStatInfo.testIdxHash( i, fieldBitmap4 ) ) ;
   }

   ASSERT_FALSE( mbStatInfo.testIdxHash( fieldBitmap5 ) ) ;
   for ( UINT32 i = 0 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
   {
      ASSERT_FALSE( mbStatInfo.testIdxHash( i, fieldBitmap5 ) ) ;
   }
   for ( UINT32 i = IXM_IDX_HASH_MAX_INDEX_NUM ;
         i < DMS_COLLECTION_MAX_INDEX ;
         ++ i )
   {
      ASSERT_TRUE( mbStatInfo.testIdxHash( i, fieldBitmap5 ) ) ;
   }

   ASSERT_FALSE( mbStatInfo.testIdxHash( fieldBitmap6 ) ) ;
   for ( UINT32 i = 0 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
   {
      ASSERT_FALSE( mbStatInfo.testIdxHash( i, fieldBitmap6 ) ) ;
   }
   for ( UINT32 i = IXM_IDX_HASH_MAX_INDEX_NUM ;
         i < DMS_COLLECTION_MAX_INDEX ;
         ++ i )
   {
      ASSERT_TRUE( mbStatInfo.testIdxHash( i, fieldBitmap6 ) ) ;
   }
}

TEST( generator, test_drop_index )
{
   dmsMBStatInfo mbStatInfo ;
   mbStatInfo.setIdxHash( 0, "_id" ) ;
   mbStatInfo.setIdxHash( 1, "a" ) ;
   mbStatInfo.setIdxHash( 1, "b" ) ;
   mbStatInfo.setIdxHash( 2, "c" ) ;
   mbStatInfo.setIdxHash( 2, "d" ) ;

   cout << "cl: " << mbStatInfo._clIdxHashBitmap.toString() << endl ;
   cout << "idx 0: " << mbStatInfo._idxHashBitmaps[ 0 ].toString() << endl ;
   cout << "idx 1: " << mbStatInfo._idxHashBitmaps[ 1 ].toString() << endl ;
   cout << "idx 2: " << mbStatInfo._idxHashBitmaps[ 2 ].toString() << endl ;

   ixmIdxHashBitmap fieldBitmap1 ;
   fieldBitmap1.setFieldBit( "_id" ) ;

   ixmIdxHashBitmap fieldBitmap2 ;
   fieldBitmap2.setFieldBit( "a" ) ;
   fieldBitmap2.setFieldBit( "b" ) ;

   ixmIdxHashBitmap fieldBitmap3 ;
   fieldBitmap3.setFieldBit( "c" ) ;
   fieldBitmap3.setFieldBit( "d" ) ;

   ixmIdxHashBitmap fieldBitmap4 ;
   fieldBitmap4.setFieldBit( "_id" ) ;
   fieldBitmap4.setFieldBit( "a" ) ;
   fieldBitmap4.setFieldBit( "b" ) ;
   fieldBitmap4.setFieldBit( "c" ) ;
   fieldBitmap4.setFieldBit( "d" ) ;

   ixmIdxHashBitmap fieldBitmap5 ;
   fieldBitmap5.setFieldBit( "_id" ) ;
   fieldBitmap5.setFieldBit( "c" ) ;
   fieldBitmap5.setFieldBit( "d" ) ;

   ASSERT_TRUE( mbStatInfo._clIdxHashBitmap.isEqual( fieldBitmap4 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashBitmaps[ 0 ].isEqual( fieldBitmap1 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashBitmaps[ 1 ].isEqual( fieldBitmap2 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashBitmaps[ 2 ].isEqual( fieldBitmap3 ) ) ;
   for ( UINT32 i = 3 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
   {
      ASSERT_TRUE( mbStatInfo._idxHashBitmaps[ i ].isEmpty() ) ;
   }

   // unset index 1
   mbStatInfo.unsetIdxHash( 1 ) ;

   ASSERT_TRUE( mbStatInfo._clIdxHashBitmap.isEmpty() ) ;
   ASSERT_TRUE( mbStatInfo._idxHashBitmaps[ 0 ].isEqual( fieldBitmap1 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashBitmaps[ 1 ].isEqual( fieldBitmap3 ) ) ;
   for ( UINT32 i = 2 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
   {
      ASSERT_TRUE( mbStatInfo._idxHashBitmaps[ i ].isEmpty() ) ;
   }

   // merge bitmaps of indexes into collection
   mbStatInfo.mergeIdxHash( 0 ) ;
   mbStatInfo.mergeIdxHash( 1 ) ;

   ASSERT_TRUE( mbStatInfo._clIdxHashBitmap.isEqual( fieldBitmap5 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashBitmaps[ 0 ].isEqual( fieldBitmap1 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashBitmaps[ 1 ].isEqual( fieldBitmap3 ) ) ;
   for ( UINT32 i = 2 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
   {
      ASSERT_TRUE( mbStatInfo._idxHashBitmaps[ i ].isEmpty() ) ;
   }
}

TEST( generator, test_add_index )
{
   dmsMBStatInfo mbStatInfo ;
   mbStatInfo.setIdxHash( 0, "_id" ) ;
   mbStatInfo.setIdxHash( 1, "a" ) ;
   mbStatInfo.setIdxHash( 1, "b" ) ;

   cout << "cl: " << mbStatInfo._clIdxHashBitmap.toString() << endl ;
   cout << "idx 0: " << mbStatInfo._idxHashBitmaps[ 0 ].toString() << endl ;
   cout << "idx 1: " << mbStatInfo._idxHashBitmaps[ 1 ].toString() << endl ;

   ixmIdxHashBitmap fieldBitmap1 ;
   fieldBitmap1.setFieldBit( "_id" ) ;

   ixmIdxHashBitmap fieldBitmap2 ;
   fieldBitmap2.setFieldBit( "a" ) ;
   fieldBitmap2.setFieldBit( "b" ) ;

   ixmIdxHashBitmap fieldBitmap3 ;
   fieldBitmap3.setFieldBit( "c" ) ;
   fieldBitmap3.setFieldBit( "d" ) ;

   ixmIdxHashBitmap fieldBitmap4 ;
   fieldBitmap4.setFieldBit( "_id" ) ;
   fieldBitmap4.setFieldBit( "a" ) ;
   fieldBitmap4.setFieldBit( "b" ) ;

   ixmIdxHashBitmap fieldBitmap5 ;
   fieldBitmap5.setFieldBit( "_id" ) ;
   fieldBitmap5.setFieldBit( "a" ) ;
   fieldBitmap5.setFieldBit( "b" ) ;
   fieldBitmap5.setFieldBit( "c" ) ;
   fieldBitmap5.setFieldBit( "d" ) ;

   ASSERT_TRUE( mbStatInfo._clIdxHashBitmap.isEqual( fieldBitmap4 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashBitmaps[ 0 ].isEqual( fieldBitmap1 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashBitmaps[ 1 ].isEqual( fieldBitmap2 ) ) ;
   for ( UINT32 i = 2 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
   {
      ASSERT_TRUE( mbStatInfo._idxHashBitmaps[ i ].isEmpty() ) ;
   }

   // set index 2
   mbStatInfo.unsetIdxHash( 2 ) ;

   ASSERT_TRUE( mbStatInfo._clIdxHashBitmap.isEmpty() ) ;
   ASSERT_TRUE( mbStatInfo._idxHashBitmaps[ 0 ].isEqual( fieldBitmap1 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashBitmaps[ 1 ].isEqual( fieldBitmap2 ) ) ;
   for ( UINT32 i = 2 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
   {
      ASSERT_TRUE( mbStatInfo._idxHashBitmaps[ i ].isEmpty() ) ;
   }

   // merge bitmaps of indexes into collection
   mbStatInfo.mergeIdxHash( 0 ) ;
   mbStatInfo.mergeIdxHash( 1 ) ;
   mbStatInfo.setIdxHash( 2, "c" ) ;
   mbStatInfo.setIdxHash( 2, "d" ) ;

   ASSERT_TRUE( mbStatInfo._clIdxHashBitmap.isEqual( fieldBitmap5 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashBitmaps[ 0 ].isEqual( fieldBitmap1 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashBitmaps[ 1 ].isEqual( fieldBitmap2 ) ) ;
   ASSERT_TRUE( mbStatInfo._idxHashBitmaps[ 2 ].isEqual( fieldBitmap3 ) ) ;
   for ( UINT32 i = 3 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
   {
      ASSERT_TRUE( mbStatInfo._idxHashBitmaps[ i ].isEmpty() ) ;
   }
}

TEST( generator, test_set )
{
   INT32 rc = SDB_OK ;
   mthModifier modifier ;

   ixmIdxHashBitmap bitmap ;
   bitmap.setFieldBit( "a" ) ;
   bitmap.setFieldBit( "b" ) ;
   bitmap.setFieldBit( "c" ) ;
   bitmap.setFieldBit( "d" ) ;

   BSONObj updator = BSON( "$set" << BSON( "a" << 1 << "b" << 1 ) <<
                           "$unset" << BSON( "c" << 1 << "d" << 1 ) ) ;
   rc = modifier.loadPattern( updator, NULL, TRUE, NULL, TRUE,
                              DPS_LOG_WRITE_MOD_INCREMENT, TRUE ) ;
   ASSERT_TRUE( SDB_OK == rc ) ;
   ASSERT_TRUE( modifier.getIdxHashBitmap().isEqual( bitmap ) ) ;
}

TEST( generator, test_rename )
{
   INT32 rc = SDB_OK ;
   mthModifier modifier ;

   ixmIdxHashBitmap bitmap ;
   bitmap.setFieldBit( "a" ) ;
   bitmap.setFieldBit( "b" ) ;

   BSONObj updator = BSON( "$rename" << BSON( "a" << "b" ) ) ;
   rc = modifier.loadPattern( updator, NULL, TRUE, NULL, TRUE,
                              DPS_LOG_WRITE_MOD_INCREMENT, TRUE ) ;
   ASSERT_TRUE( SDB_OK == rc ) ;
   ASSERT_TRUE( modifier.getIdxHashBitmap().isEqual( bitmap ) ) ;
}

TEST( generator, test_replace )
{
   INT32 rc = SDB_OK ;
   mthModifier modifier ;

   BSONObj updator = BSON( "$replace" << BSON( "a" << 1 << "b" << 1 ) ) ;
   rc = modifier.loadPattern( updator, NULL, TRUE, NULL, TRUE,
                              DPS_LOG_WRITE_MOD_INCREMENT, TRUE ) ;
   ASSERT_TRUE( SDB_OK == rc ) ;
   ASSERT_TRUE( modifier.getIdxHashBitmap().isFull() ) ;
}
