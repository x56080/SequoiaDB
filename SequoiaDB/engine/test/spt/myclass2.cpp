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

   Source File Name = myclass2.cpp

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

#include "myclass2.hpp"
#include <iostream>

using namespace std ;

JS_MEMBER_FUNC_DEFINE( myclass2, func)
JS_CONSTRUCT_FUNC_DEFINE( myclass2, construct)
JS_DESTRUCT_FUNC_DEFINE(myclass2, destruct)

JS_BEGIN_MAPPING(myclass2, "myjsclass2")
  JS_ADD_MEMBER_FUNC("func", func)
  JS_ADD_CONSTRUCT_FUNC(construct)
  JS_ADD_DESTRUCT_FUNC(destruct)
JS_MAPPING_END()

myclass2::myclass2()
{
   cout << "myclass2::myclass2" << endl ;
}

myclass2::~myclass2()
{
   cout << "myclass2""~myclass2" << endl ;
}

INT32 myclass2::func( const _sptParamContainer &arg,
                    _sptReturnVal &rval,
                    bson::BSONObj &detail )
{
   cout << "myclass2::fun" << endl ;
   return SDB_OK ;
}

INT32 myclass2::construct( const _sptParamContainer &arg,
                          _sptReturnVal &rval,
                           bson::BSONObj &detail)
{
   cout << "myclass2::construct" << endl;
   return SDB_OK ;
}

INT32 myclass2::destruct()
{
   cout << "myclass2::destruct" << endl ;
   return SDB_OK ;
}
