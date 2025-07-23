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

   Source File Name = coordSequenceOperator.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/10/2020  LSQ Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORD_SEQUENCE_OPERATOR_HPP__
#define COORD_SEQUENCE_OPERATOR_HPP__

#include "coordOperator.hpp"

namespace engine
{
   /*
      _coordSeqFetchOperator define
   */
   class _coordSeqFetchOperator : public _coordOperator
   {
      public:
         _coordSeqFetchOperator() ;
         virtual ~_coordSeqFetchOperator() ;

         virtual const CHAR* getName() const ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;

   } ;
   typedef _coordSeqFetchOperator coordSeqFetchOperator ;
}

#endif // COORD_SEQUENCE_OPERATOR_HPP__
