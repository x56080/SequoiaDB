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
#include "utilStr.hpp"
#include "ossUtil.hpp"
#include "gtest/gtest.h"

using namespace engine ;

// Test str to lower
TEST( utilStrTest, strToLower )
{
   const CHAR *testStr = "TEST_STR_TO_LOWER" ;
   CHAR result[30] = { 0 } ;
   utilStrToLower( testStr, result, sizeof( result ) ) ;
   ASSERT_STREQ( result, "test_str_to_lower" ) ;
}

// Test str to upper
TEST( utilStrTest, strToUpper )
{
   const CHAR *testStr = "test_str_to_upper" ;
   CHAR result[30] = { 0 } ;
   utilStrToUpper( testStr, result, sizeof( result ) ) ;
   ASSERT_STREQ( result, "TEST_STR_TO_UPPER" ) ;
}
