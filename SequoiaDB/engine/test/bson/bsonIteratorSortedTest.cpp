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