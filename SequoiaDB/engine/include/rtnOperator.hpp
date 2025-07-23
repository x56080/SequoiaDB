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

   Source File Name = rtnOperator.hpp

   Descriptive Name = Base class for rtn operators.

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/04/2017  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_OPERATOR_HPP__
#define RTN_OPERATOR_HPP__

#include "oss.hpp"
#include "pmdEDU.hpp"
#include "rtnCB.hpp"
#include "rtnQueryOptions.hpp"

namespace engine
{
   // Base class for rtn Operators.
   class _rtnOperator : public SDBObject
   {
      public:
         _rtnOperator() ;
         virtual ~_rtnOperator() ;
      public:
         virtual INT32 run( const rtnQueryOptions &options,
                            pmdEDUCB *eduCB,
                            SDB_RTNCB *rtnCB,
                            INT64 &contextID,
                            rtnContextBase **ppContext,
                            BOOLEAN enablePrefetch ) = 0 ;
   } ;
   typedef _rtnOperator rtnOperator ;
}

#endif /* RTN_OPERATOR_HPP__ */

