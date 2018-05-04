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

   Source File Name = myclass.cpp

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

#include "myclass.hpp"
#include <iostream>

using namespace std ;

JS_MEMBER_FUNC_DEFINE( myclass, func)
JS_CONSTRUCT_FUNC_DEFINE( myclass, construct)
JS_DESTRUCT_FUNC_DEFINE(myclass, destruct)

JS_BEGIN_MAPPING(myclass, "myjsclass")
  JS_ADD_MEMBER_FUNC("func", func)
  JS_ADD_CONSTRUCT_FUNC(construct)
  JS_ADD_DESTRUCT_FUNC(destruct)
JS_MAPPING_END()

myclass::myclass()
{
   cout << "myclass::myclass" << endl ;
}

myclass::~myclass()
{
   cout << "myclass""~myclass" << endl ;
}

INT32 myclass::func( const _sptParamContainer &arg,
                    _sptReturnVal &rval,
                    bson::BSONObj &detail )
{
   cout << "myclass::fun" << endl ;
   return SDB_OK ;
}

INT32 myclass::construct( const _sptParamContainer &arg,
                          _sptReturnVal &rval,
                           bson::BSONObj &detail)
{
   cout << "myclass::construct" << endl;
   return SDB_OK ;
}

INT32 myclass::destruct()
{
   cout << "myclass::destruct" << endl ;
   return SDB_OK ;
}
