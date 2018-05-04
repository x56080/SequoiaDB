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
*******************************************************************************/

#include "ossTypes.hpp"
#include <gtest/gtest.h>
#include "spdFMPMgr.hpp"
#include "spdFMP.hpp"

using namespace engine ;

TEST(spdTest, spdTest_1)
{
   INT32 rc = SDB_OK ;
   spdFMPMgr fmpMgr ;
   rc = fmpMgr.init() ;
   ASSERT_TRUE( rc == SDB_OK ) ;

   spdFMP *fmp = NULL ;
   rc = fmpMgr.getFMP( fmp ) ;
   ASSERT_TRUE( rc == SDB_OK ) ;
   getchar () ;
}
