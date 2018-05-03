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

   Source File Name = optQgmSpecStrategy.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          27/04/2013  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OPT_QGM_SPEC_STRATEGY_HPP_
#define OPT_QGM_SPEC_STRATEGY_HPP_

#include "optQgmStrategy.hpp"
#include "qgmConditionNode.hpp"

namespace engine
{

   class _optQgmSortAggrSty : public optQgmStrategyBase
   {
      public:
         _optQgmSortAggrSty() {}
         virtual ~_optQgmSortAggrSty () {}

      public:
         virtual INT32  calcResult( qgmOprUnit *oprUnit,
                                    qgmOptiTreeNode *curNode,
                                    qgmOptiTreeNode *subNode,
                                    OPT_QGM_SS_RESULT &result ) ;

         virtual const CHAR* strategyName() const ;

   };
   typedef _optQgmSortAggrSty optQgmSortAggrSty ;

   class _optQgmSortFilterSty : public optQgmStrategyBase
   {
      public:
         _optQgmSortFilterSty() {}
         virtual ~_optQgmSortFilterSty() {}

      public:
         virtual INT32  calcResult( qgmOprUnit *oprUnit,
                                    qgmOptiTreeNode *curNode,
                                    qgmOptiTreeNode *subNode,
                                    OPT_QGM_SS_RESULT &result ) ;

         virtual const CHAR* strategyName() const ;

   } ;
   typedef _optQgmSortFilterSty optQgmSortFilterSty ;

   class _optQgmSortJoinSty : public optQgmStrategyBase
   {
      public:
         _optQgmSortJoinSty() {}
         virtual ~_optQgmSortJoinSty() {}

      public:
         virtual INT32  calcResult( qgmOprUnit *oprUnit,
                                    qgmOptiTreeNode *curNode,
                                    qgmOptiTreeNode *subNode,
                                    OPT_QGM_SS_RESULT &result ) ;

         virtual const CHAR* strategyName() const ;

   };
   typedef _optQgmSortJoinSty optQgmSortJoinSty ;

   class _optQgmFilterSortSty : public optQgmStrategyBase
   {
      public:
         _optQgmFilterSortSty() {}
         virtual ~_optQgmFilterSortSty() {}

      public:
         virtual INT32  calcResult( qgmOprUnit *oprUnit,
                                    qgmOptiTreeNode *curNode,
                                    qgmOptiTreeNode *subNode,
                                    OPT_QGM_SS_RESULT &result ) ;

         virtual const CHAR* strategyName() const ;

   };
   typedef _optQgmFilterSortSty optQgmFilterSortSty ;

   class _optQgmFilterFilterSty : public optQgmStrategyBase
   {
      public:
         _optQgmFilterFilterSty() {}
         virtual ~_optQgmFilterFilterSty() {}

      public:
         virtual INT32  calcResult( qgmOprUnit *oprUnit,
                                    qgmOptiTreeNode *curNode,
                                    qgmOptiTreeNode *subNode,
                                    OPT_QGM_SS_RESULT &result ) ;

         virtual const CHAR* strategyName() const ;

   };
   typedef _optQgmFilterFilterSty optQgmFilterFilterSty ;

   class _optQgmFilterAggrSty : public optQgmStrategyBase
   {
      public:
         _optQgmFilterAggrSty() {}
         virtual ~_optQgmFilterAggrSty() {}

      public:
         virtual INT32  calcResult( qgmOprUnit *oprUnit,
                                    qgmOptiTreeNode *curNode,
                                    qgmOptiTreeNode *subNode,
                                    OPT_QGM_SS_RESULT &result ) ;

         virtual const CHAR* strategyName() const ;

   };
   typedef _optQgmFilterAggrSty optQgmFilterAggrSty ;

   class _optQgmFilterJoinSty : public optQgmStrategyBase
   {
      public:
         _optQgmFilterJoinSty() {}
         virtual ~_optQgmFilterJoinSty() {}

      public:
         virtual INT32  calcResult( qgmOprUnit *oprUnit,
                                    qgmOptiTreeNode *curNode,
                                    qgmOptiTreeNode *subNode,
                                    OPT_QGM_SS_RESULT &result ) ;

         virtual const CHAR* strategyName() const ;

   };
   typedef _optQgmFilterJoinSty optQgmFilterJoinSty ;

   class _optQgmAggrFilterSty : public optQgmStrategyBase
   {
      public:
         _optQgmAggrFilterSty() {}
         virtual ~_optQgmAggrFilterSty() {}

      public:
         virtual INT32  calcResult( qgmOprUnit *oprUnit,
                                    qgmOptiTreeNode *curNode,
                                    qgmOptiTreeNode *subNode,
                                    OPT_QGM_SS_RESULT &result ) ;

         virtual const CHAR* strategyName() const ;

   };
   typedef _optQgmAggrFilterSty optQgmAggrFilterSty ;

   class _optQgmFilterScanSty : public optQgmStrategyBase
   {
   public:
      _optQgmFilterScanSty() {}
      virtual ~_optQgmFilterScanSty() {}

   public:
      virtual INT32  calcResult( qgmOprUnit *oprUnit,
                                    qgmOptiTreeNode *curNode,
                                    qgmOptiTreeNode *subNode,
                                    OPT_QGM_SS_RESULT &result ) ;

      virtual const CHAR* strategyName() const ;
   } ;
   typedef class _optQgmFilterScanSty optQgmFilterScanSty ;

   /////////////////////////////////////////////////////////////////////////////
   // tool functions
   /////////////////////////////////////////////////////////////////////////////

   BOOLEAN isCondSameRele( qgmConditionNode *condNode, BOOLEAN allowEmpty = TRUE ) ;

   BOOLEAN isCondIncludedNull( qgmConditionNode *condNode ) ;

}

#endif //OPT_QGM_SPEC_STRATEGY_HPP_

