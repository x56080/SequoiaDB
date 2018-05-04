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

   Source File Name = qgmExtendSelectPlan.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef QGMEXTENDSELECTPLAN_HPP_
#define QGMEXTENDSELECTPLAN_HPP_

#include "qgmExtendPlan.hpp"
#include "qgmOptiAggregation.hpp"

namespace engine
{
   const UINT32 QGM_EXTEND_ORDERFILTER = 0 ;
   const UINT32 QGM_EXTEND_ORDERBY = 1 ;
   const UINT32 QGM_EXTEND_SPLITBY = 2 ;
   const UINT32 QGM_EXTEND_AGGR = 3 ;
   const UINT32 QGM_EXTEND_GROUPBY = 4 ;
   const UINT32 QGM_EXTEND_LOCAL = 5 ;

   class _qgmExtendSelectPlan : public _qgmExtendPlan
   {
   public:
      _qgmExtendSelectPlan() ;
      virtual ~_qgmExtendSelectPlan() ;

      void    clearConstraint() { _limit = -1 ; _skip = 0 ; }

   private:
      INT32 _extend( UINT32 id,
                     qgmOptiTreeNode *&extended ) ;

   protected:

   public:
      qgmAggrSelectorVec _funcSelector ;
      qgmOPFieldVec _groupby ;
      qgmOPFieldVec _orderby ;
      qgmOPFieldVec _original ;
      qgmDbAttr _splitby ;
      INT64         _limit ;
      INT64         _skip ;

   } ;

   typedef class _qgmExtendSelectPlan qgmExtendSelectPlan ;
}

#endif

