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

   Source File Name = bsonIteratorSortedTest.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed = 2023/2/15

*******************************************************************************/
#include "ossTypes.hpp"
#include "../bson/bson.h"
#include "ossUtil.hpp"
#include "ossMemPool.hpp"

#include <gtest/gtest.h>
#include <iostream>

using namespace std;
using namespace bson;

static void genString ( CHAR *str, UINT32 strLength, BOOLEAN random )
{
    UINT32 len = strLength ;

    if ( random )
    {
        len = ossRand() % strLength + 1 ;
    }

    for ( UINT32 i = 0; i < len; i++ )
    {
        if ( ossRand() % 2 == 1 )
            str[i] = 'a' + ossRand() % 26 ;
        else
            str[i] = 'A' + ossRand() % 26 ;
    }
    str[len] = '\0' ;
}

static void genBsonObj( UINT32 nums, UINT32 strLength, BSONObj& o )
{
    BSONObjBuilder builder ;

    CHAR* pField = NULL, * pVal = NULL ;
    pField = (CHAR*)SDB_OSS_MALLOC( strLength + 1 ) ;
    pVal = (CHAR*)SDB_OSS_MALLOC( strLength + 1 ) ;

    while ( nums-- )
    {
        genString( pField, strLength, FALSE ) ;
        genString( pVal, strLength, TRUE ) ;
        builder.append( pField, pVal ) ;
    }

    o = builder.obj().getOwned() ;
    SDB_OSS_FREE( pField ) ;
    SDB_OSS_FREE( pVal ) ;
}

static void perfTestMain( UINT64 repeatKTimes, UINT32 bsonFieldNums )
{
    BSONObj obj ;
    UINT64 beginTime = 0, endTime = 0 ;
    UINT64 repeatTimes = 1000 * repeatKTimes ;

    genBsonObj( bsonFieldNums, 10, obj ) ;
    beginTime = ossGetCurrentMicroseconds() ;
    while ( repeatTimes-- )
    {
        BSONObjIteratorSorted itrSorted( obj ) ;
    }
    endTime = ossGetCurrentMicroseconds() ;
    cout << "Sorted time: ( " << ( endTime - beginTime ) / 1000 << " ms )" << endl ;
}

static void functionTestMain()
{
    BSONObj obj ;
    ossPoolSet<ossPoolString> set ;
    genBsonObj( 1000, 10, obj ) ;

    BSONObjIterator itr( obj ) ;
    while ( itr.more() )
    {
        BSONElement e = itr.next() ;
        if ( e.eoo() )
            break ;
        set.insert( e.fieldName() ) ;
    }

    BSONObjIteratorSorted sortedItr( obj ) ;
    ossPoolSet<ossPoolString>::const_iterator setItr = set.begin() ;

    // Compare
    for ( int cnt = 0; cnt < obj.nFields(); ++cnt )
    {
        BSONElement e = sortedItr.next() ;
        ASSERT_TRUE( 0 == ossStrcmp( e.fieldName(), (*setItr).c_str() ) ) ;
        ++setItr ;
    }
}

TEST( iteratorSorted, function_test )
{
    functionTestMain() ;
}

TEST( iteratorSorted, perf_test_10_99 )
{
    perfTestMain( 10, 99 ) ;
}

TEST( iteratorSorted, perf_test_100_99 )
{
    perfTestMain( 100, 99 ) ;
}

TEST( iteratorSorted, perf_test_1000_99 )
{
    perfTestMain( 1000, 99 ) ;
}

TEST( iteratorSorted, perf_test_10_200 )
{
    perfTestMain( 10, 200 ) ;
}

TEST( iteratorSorted, perf_test_100_200 )
{
    perfTestMain( 100, 200 ) ;
}

TEST( iteratorSorted, perf_test_1000_200 )
{
    perfTestMain( 1000, 200 ) ;
}

TEST( iteratorSorted, perf_test_10_500 )
{
    perfTestMain( 10, 500 ) ;
}

TEST( iteratorSorted, perf_test_100_500 )
{
    perfTestMain( 100, 500 ) ;
}

TEST( iteratorSorted, perf_test_1000_500 )
{
    perfTestMain( 1000, 500 ) ;
}