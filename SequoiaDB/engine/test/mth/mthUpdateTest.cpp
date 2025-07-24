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

   Source File Name = mthUpdateTest.cpp

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
#include "mthModifier.hpp"
#include "../bson/bson.h"

#include "gtest/gtest.h"
#include <iostream>

using namespace std ;
using namespace engine ;
using namespace bson ;

TEST( generator, test_inc_long )
{
   INT32 rc = SDB_OK ;
   mthModifier modifier ;
   BSONObjBuilder updateBuilder ;

   BSONObjBuilder incBuilder( updateBuilder.subobjStart( "$inc" ) ) ;
   incBuilder.append( "a", 1LL ) ;
   incBuilder.doneFast() ;

   BSONObj updator = updateBuilder.obj() ;
   rc = modifier.loadPattern( updator ) ;
   ASSERT_TRUE( SDB_OK == rc ) ;

   {
      // result is number int
      BSONObj source = BSON( "a" << 1 ) ;
      BSONObj target ;
      rc = modifier.modify( source, target ) ;
      ASSERT_TRUE( SDB_OK == rc ) ;
      ASSERT_TRUE( NumberInt == target.firstElement().type() ) ;
   }

   {
      // result is overflow to number long
      BSONObj source = BSON( "a" << OSS_SINT32_MAX ) ;
      BSONObj target ;
      rc = modifier.modify( source, target ) ;
      ASSERT_TRUE( SDB_OK == rc ) ;
      ASSERT_TRUE( NumberLong == target.firstElement().type() ) ;
   }

   {
      // result is double
      BSONObj source = BSON( "a" << 1.0 ) ;
      BSONObj target ;
      rc = modifier.modify( source, target ) ;
      ASSERT_TRUE( SDB_OK == rc ) ;
      ASSERT_TRUE( NumberDouble == target.firstElement().type() ) ;
   }
}
