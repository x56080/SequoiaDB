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

   Source File Name = myclass2.hpp

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

#ifndef MYCLASS2_HPP_
#define MYCLASS2_HPP_

#include "sptApi.hpp"

using namespace engine;

class myclass2 : public SDBObject
{
JS_DECLARE_CLASS(myclass2)

public:
   myclass2() ;
   virtual ~myclass2() ;

public:
   INT32 construct( const _sptParamContainer &arg,
                    _sptReturnVal &rval,
                    bson::BSONObj &detail) ;

   INT32 func( const _sptParamContainer &arg,
               _sptReturnVal &rval,
               bson::BSONObj &detail ) ;

   INT32 destruct() ; 
} ;

#endif

