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

   Source File Name = coordSqlOperator.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/02/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORD_SQL_OPERATOR_HPP_
#define COORD_SQL_OPERATOR_HPP_

#include "coordOperator.hpp"

namespace engine
{
   /*
      _coordSqlOperator define
   */
   class _coordSqlOperator : public _coordOperator
   {
      public:
         _coordSqlOperator() ;
         virtual ~_coordSqlOperator() ;

         virtual const CHAR* getName() const ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;

         virtual BOOLEAN      needRollback() const ;

      private:
         BOOLEAN        _needRollback ;

   } ;
   typedef _coordSqlOperator coordSqlOperator ;

}

#endif // COORD_SQL_OPERATOR_HPP_

