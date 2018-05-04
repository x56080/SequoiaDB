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

   Source File Name = rtnSQLFirst.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTNSQLFIRST_HPP_
#define RTNSQLFIRST_HPP_

#include "rtnSQLFunc.hpp"

namespace engine
{

   class _rtnSQLFirst : public _rtnSQLFunc
   {
   public:
      _rtnSQLFirst( const CHAR *pName ) ;
      virtual ~_rtnSQLFirst() ;

   public:
      virtual INT32 result( BSONObjBuilder &builder ) ;

      void firstInAllRecord()
      {
         _firstInAll = TRUE ;
      }

   private:
      virtual INT32 _push( const RTN_FUNC_PARAMS &param ) ;

   private:
      BSONObj _obj ;
      BSONElement _ele ;
      BOOLEAN _firstInAll ;
   } ;

   typedef class _rtnSQLFirst rtnSQLFirst ;
}

#endif

