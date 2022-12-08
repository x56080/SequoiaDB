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
