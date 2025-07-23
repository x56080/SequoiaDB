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

   Source File Name = coordAggrOperator.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/18/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORD_AGGR_OPERATOR_HPP__
#define COORD_AGGR_OPERATOR_HPP__

#include "coordOperator.hpp"

namespace engine
{
   /*
      _coordAggrOperator define
   */
   class _coordAggrOperator : public _coordOperator
   {
      public:
         _coordAggrOperator() ;
         virtual ~_coordAggrOperator() ;

         virtual const CHAR* getName() const ;

         virtual INT32        execute( MsgHeader *pMsg,
                                       pmdEDUCB *cb,
                                       INT64 &contextID,
                                       rtnContextBuf *buf ) ;

         virtual BOOLEAN      needRollback() const ;

      private:
         BOOLEAN              _needRollback ;

   } ;
   typedef _coordAggrOperator coordAggrOperator ;

}

#endif // COORD_AGGR_OPERATOR_HPP__
