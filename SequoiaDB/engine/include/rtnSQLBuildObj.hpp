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

   Source File Name = rtnSQLBuildObj.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/16/2013  JHL  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTNSQLBUILDOBJ_HPP__
#define RTNSQLBUILDOBJ_HPP__

#include "rtnSQLFunc.hpp"
#include "../bson/bsonobj.h"

namespace engine
{
   class rtnSQLBuildObj : public _rtnSQLFunc
   {
   public:
      rtnSQLBuildObj( const CHAR *pName );

      virtual ~rtnSQLBuildObj();

      virtual INT32 result( bson::BSONObjBuilder &builder );

      virtual BOOLEAN isAggr() const { return FALSE ; }

   private:
      virtual INT32 _push( const RTN_FUNC_PARAMS &param );

   private:
      bson::BSONObj     _obj;
      BOOLEAN           _hasData;
   } ;
}

#endif // RTNSQLBUILDOBJ_HPP__

