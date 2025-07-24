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
#include "ossTypes.hpp"
#include <gtest/gtest.h>
#include <iostream>

#include "optStatUnit.hpp"

using namespace std;
using namespace engine;
using namespace bson ;

TEST(optIndexPathTest, empty)
{
   optIndexPathEncoder encoder ;
   std::cout << encoder.getPath() << std::endl ;
   ASSERT_TRUE( encoder.getPath() == ossPoolString( "" ) ) ;
}

TEST(optIndexPathTest, prefix_1)
{
   optIndexPathEncoder encoderR, encoderL ;
   encoderR.append( "a", FALSE ) ;
   encoderR.append( "b", TRUE ) ;
   encoderL.append( "a", FALSE ) ;
   std::cout << encoderR.getPath() << " " << encoderL.getPath() << std::endl ;
   ASSERT_TRUE( encoderR.getPath() == encoderL.getPath() ) ;
}

TEST(optIndexPathTest, prefix_2)
{
   optIndexPathEncoder encoderR, encoderL ;
   encoderR.append( "a", FALSE ) ;
   encoderR.append( "b", TRUE ) ;
   encoderL.append( "a", FALSE ) ;
   encoderL.append( "c", TRUE ) ;
   std::cout << encoderR.getPath() << " " << encoderL.getPath() << std::endl ;
   ASSERT_TRUE( encoderR.getPath() == encoderL.getPath() ) ;
}

TEST(optIndexPathTest, name_with_underline)
{
   optIndexPathEncoder encoderR, encoderL ;
   encoderR.append( "a", FALSE ) ;
   encoderR.append( "b", FALSE ) ;
   encoderL.append( "a_b", FALSE ) ;
   std::cout << encoderR.getPath() << " " << encoderL.getPath() << std::endl ;
   ASSERT_TRUE( encoderR.getPath() != encoderL.getPath() ) ;
}
