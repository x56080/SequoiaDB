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

   Source File Name = bsonBuilderTest.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#include "ossTypes.hpp"
#include "../bson/bson.h"

#include "gtest/gtest.h"
#include <iostream>

using namespace std ;
using namespace bson ;

TEST( generator, test_abandon )
{
   BSONObjBuilder builder ;
   builder.append( "a", 1 ) ;

   {
      UINT32 origResvLen = builder.bb().getReserveBytes() ;
      UINT32 origLen = builder.bb().len() ;
      BSONObjBuilder subBuilder( builder.subobjStart( "b" ) ) ;
      subBuilder.append( "c", 1 ) ;
      subBuilder.abandon() ;
      builder.bb().setlen( origLen );
      builder.bb().setReserveBytes( origResvLen );
   }

   builder.append( "b", 1 ) ;
   BSONObj ob = builder.obj() ;
   ASSERT_TRUE( ob.shallowEqual( BSON( "a" << 1 << "b" << 1 ) ) ) ;
}

TEST( generator, test_abandon_arr )
{
   BSONObjBuilder builder ;
   builder.append( "a", 1 ) ;

   {
      UINT32 origResvLen = builder.bb().getReserveBytes() ;
      UINT32 origLen = builder.bb().len() ;
      BSONArrayBuilder subBuilder( builder.subarrayStart( "b" ) ) ;
      subBuilder.append( 1 ) ;
      subBuilder.abandon() ;
      builder.bb().setlen( origLen );
      builder.bb().setReserveBytes( origResvLen );
   }

   builder.append( "b", 1 ) ;
   BSONObj ob = builder.obj() ;
   ASSERT_TRUE( ob.shallowEqual( BSON( "a" << 1 << "b" << 1 ) ) ) ;
}

