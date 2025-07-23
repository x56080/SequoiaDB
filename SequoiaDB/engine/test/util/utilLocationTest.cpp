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

   Source File Name = utilLocationTest.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/26/2022  HYQ  Initial Draft

   Last Changed =

*******************************************************************************/
#include"ossTypes.hpp"
#include<gtest/gtest.h>

#include "utilLocation.hpp"
#include <iostream>


namespace engine
{

    /*
   Name: src_select_test1
   Description:
      location affinity test
   Expected Result:
      affinitive in one of below cases:
      1. location1 prefix == location2 prefix
      2. location1 prefix == location2
      3. location1 == location2
   */


   TEST(utilLocationTest, utilAffinity_1)
   {
      ASSERT_TRUE ( FALSE == utilCalAffinity( NULL, NULL ) ) ;
      ASSERT_TRUE ( FALSE == utilCalAffinity( NULL, "A.b") ) ;
      ASSERT_TRUE ( FALSE == utilCalAffinity( "A.b", NULL ) ) ;
      ASSERT_TRUE ( FALSE == utilCalAffinity( "", "" ) ) ;
      ASSERT_TRUE ( FALSE == utilCalAffinity( "A.b", "" ) ) ;
      ASSERT_TRUE ( FALSE == utilCalAffinity( "", "A.C" ) ) ;
      ASSERT_TRUE ( FALSE == utilCalAffinity( "A.a", ".a" ) ) ;
      ASSERT_TRUE ( FALSE == utilCalAffinity( ".a", "A.a" ) ) ;
      ASSERT_TRUE ( FALSE == utilCalAffinity( ".a", ".b" ) ) ;
      ASSERT_TRUE ( FALSE == utilCalAffinity( "A.a", "....." ) ) ;
      ASSERT_TRUE ( TRUE == utilCalAffinity( "A.b", "A.a" ) ) ;
      ASSERT_TRUE ( FALSE == utilCalAffinity( "A.b", "B.a" ) ) ;
      ASSERT_TRUE ( TRUE == utilCalAffinity( "A.b", "A.a.b.c" ) ) ;
      ASSERT_TRUE ( TRUE == utilCalAffinity( "A.b", "A" ) ) ;
      ASSERT_TRUE ( FALSE == utilCalAffinity( "AB.b", "A.b" ) ) ;
      ASSERT_TRUE ( TRUE == utilCalAffinity( "A", "A.b" ) ) ;
      ASSERT_TRUE ( TRUE == utilCalAffinity( "A", "A" ) ) ;
      ASSERT_TRUE ( FALSE == utilCalAffinity( "Adafa", "A" ) ) ;
      ASSERT_TRUE ( FALSE == utilCalAffinity( "A", "Adadafa" ) ) ;
   }

}