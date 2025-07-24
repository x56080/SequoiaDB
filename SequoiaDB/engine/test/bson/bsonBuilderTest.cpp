/*******************************************************************************

<<<<<<< HEAD
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
=======
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
>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2

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
<<<<<<< HEAD
=======

>>>>>>> c4064a6f2c2dfdf2b1bf049c2f904b74db0494b2
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

