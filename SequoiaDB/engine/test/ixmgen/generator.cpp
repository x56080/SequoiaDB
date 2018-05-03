/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = generator.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/1/2014  ly  Initial Draft

   Last Changed =

*******************************************************************************/

#include "ixmIndexKey.hpp"
#include "../bson/ordering.h"
#include "sptConvertor2.hpp"

#include "gtest/gtest.h"
#include <iostream>

using namespace std ;
using namespace engine ;
using namespace bson ;

extern void getBSONRaw( const CHAR *js, CHAR **raw ) ;

TEST(generator, test1)
{
//   const CHAR *js = "{a:[{c:[{d:{e:[1,2]}}, {d:\"abc\"}]}, {c:[{d:{e:[5,6]}}, {d:{e:[7,8]}}]}], b:10}" ;
   {
   const CHAR *js = "{no:1,name:\"A\",age:2,array1:[{array2:[{array3:[{array4:[\"array5\",\"temp4\"]},\"temp3\"]},\"temp2\"]},\"temp1\"]}"; 
   CHAR *raw = NULL ;
   getBSONRaw( js, &raw ) ;
   ASSERT_TRUE( NULL != raw ) ;
   BSONObj obj( raw ) ;
   BSONObj keyDef = BSON("array1.array2.array3.array4.1" << 1 ) ;
   _ixmIndexKeyGen gen( keyDef ) ;
   Ordering order(Ordering::make(keyDef)) ;
   BSONObjSet keySet( keyDef ) ;
   BSONElement arr ;
   INT32 rc = SDB_OK ;
   rc = gen.getKeys( obj, keySet, &arr ) ;
   ASSERT_TRUE( SDB_OK == rc ) ;
   for ( BSONObjSet::const_iterator itr = keySet.begin() ;
         itr != keySet.end() ;
         itr++ )
   {
      cout << itr->toString() << endl ;
   }

   cout << "arr:" << arr.toString( true, true )  << endl ;
   }
}
