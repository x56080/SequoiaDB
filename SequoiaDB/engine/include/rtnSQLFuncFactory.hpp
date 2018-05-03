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

   Source File Name = rtnSQLFuncFactory.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTNSQLFUNCFACTORY_HPP_
#define RTNSQLFUNCFACTORY_HPP_

#include "rtnSQLFunc.hpp"

namespace engine
{
   class _rtnSQLFunc ;

   #define RTN_SQL_FUNC_COUNT                "count"
   #define RTN_SQL_FUNC_SUM                  "sum"
   #define RTN_SQL_FUNC_MIN                  "min"
   #define RTN_SQL_FUNC_MAX                  "max"
   #define RTN_SQL_FUNC_AVG                  "avg"
   #define RTN_SQL_FUNC_FIRST                "first"
   #define RTN_SQL_FUNC_LAST                 "last"
   #define RTN_SQL_FUNC_PUSH                 "push"
   #define RTN_SQL_FUNC_ADDTOSET             "addtoset"
   #define RTN_SQL_FUNC_BUILDOBJ             "buildobj"
   #define RTN_SQL_FUNC_MERGEARRAYSET        "mergearrayset"

   class _rtnSQLFuncFactory : public SDBObject
   {
   public:
      INT32 create( const CHAR *name,
                    UINT32 paramNum,
                    _rtnSQLFunc *&func ) ;
   } ;

   typedef class _rtnSQLFuncFactory rtnSQLFuncFactory ;
}

#endif

