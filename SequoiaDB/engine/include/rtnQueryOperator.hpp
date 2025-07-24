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

   Source File Name = rtnQueryOperator.hpp

   Descriptive Name = rtn query operator.

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
#ifndef RTN_QUERY_OPERATOR__
#define RTN_QUERY_OPERATOR__

#include "rtnOperator.hpp"

namespace engine
{
   // Operator for query with text search.
   class _rtnTSQueryOperator : public _rtnOperator
   {
      public:
         _rtnTSQueryOperator() ;
         virtual ~_rtnTSQueryOperator() ;

         INT32 run( const rtnQueryOptions &options,
                    pmdEDUCB *eduCB,
                    SDB_RTNCB *rtnCB,
                    INT64 &contextID,
                    rtnContextBase **ppContext,
                    BOOLEAN enablePrefetch ) ;
   } ;
   typedef _rtnTSQueryOperator rtnTSQueryOperator ;
}

#endif /* RTN_QUERY_OPERATOR__ */

