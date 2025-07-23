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

   Source File Name = coordQueryLobOperator.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/14/2019  Linyoubin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORD_QUERY_LOB_OPERATOR_HPP__
#define COORD_QUERY_LOB_OPERATOR_HPP__

#include "coordQueryOperator.hpp"

using namespace bson ;

namespace engine
{
   /*
      _coordQueryLobOperator define
   */
   class _coordQueryLobOperator : public _coordQueryOperator
   {
      public:
         _coordQueryLobOperator() ;
         virtual ~_coordQueryLobOperator() ;

      public:
         virtual INT32 doOpOnCL( coordCataSel &cataSel,
                                 const BSONObj &objMatch,
                                 coordSendMsgIn &inMsg,
                                 coordSendOptions &options,
                                 pmdEDUCB *cb,
                                 coordProcessResult &result ) ;
   } ;

   typedef _coordQueryLobOperator coordQueryLobOperator ;
}

#endif //COORD_QUERY_LOB_OPERATOR_HPP__

