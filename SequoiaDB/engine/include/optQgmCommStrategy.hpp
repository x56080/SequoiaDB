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

   Source File Name = optQgmCommStrategy.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          27/04/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OPT_QGM_COMM_STRATEGY_HPP_
#define OPT_QGM_COMM_STRATEGY_HPP_

#include "optQgmStrategy.hpp"

namespace engine
{

   class _optQgmAcceptSty : public optQgmStrategyBase
   {
      public:
         _optQgmAcceptSty() {}
         virtual ~_optQgmAcceptSty() {}

      public:
         virtual INT32  calcResult( qgmOprUnit *oprUnit,
                                    qgmOptiTreeNode *curNode,
                                    qgmOptiTreeNode *subNode,
                                    OPT_QGM_SS_RESULT &result )
         {
            result = OPT_SS_ACCEPT ;
            return SDB_OK ;
         }

         virtual const CHAR* strategyName() const
         {
            return "ACEETP-Stragegy" ;
         }
   };
   typedef _optQgmAcceptSty optQgmAcceptSty ;

   class _optQgmRefuseSty : public optQgmStrategyBase
   {
      public:
         _optQgmRefuseSty() {}
         virtual ~_optQgmRefuseSty() {}

      public:
         virtual INT32  calcResult( qgmOprUnit *oprUnit,
                                    qgmOptiTreeNode *curNode,
                                    qgmOptiTreeNode *subNode,
                                    OPT_QGM_SS_RESULT &result )
         {
            result = OPT_SS_REFUSE ;
            return SDB_OK ;
         }

         virtual const CHAR* strategyName() const
         {
            return "REFUSE-Strategy" ;
         }
   };
   typedef _optQgmRefuseSty optQgmRefuseSty ;

   class _optQgmTakeOverSty : public optQgmStrategyBase
   {
      public:
         _optQgmTakeOverSty() {}
         virtual ~_optQgmTakeOverSty() {}

      public:
         virtual INT32  calcResult( qgmOprUnit *oprUnit,
                                    qgmOptiTreeNode *curNode,
                                    qgmOptiTreeNode *subNode,
                                    OPT_QGM_SS_RESULT &result )
         {
            result = OPT_SS_TAKEOVER ;
            return SDB_OK ;
         }

         virtual const CHAR* strategyName() const
         {
            return "TAKEOVER-Strategy" ;
         }
   };
   typedef _optQgmTakeOverSty optQgmTakeOverSty ;

}

#endif //OPT_QGM_COMM_STRATEGY_HPP_

