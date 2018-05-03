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

   Source File Name = qgmExtendPlan.hpp

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

#ifndef QGMEXTENDPLAN_HPP_
#define QGMEXTENDPLAN_HPP_

#include "qgmOptiTree.hpp"
#include <queue>
#include "utilMap.hpp"

namespace engine
{

   typedef INT32 QGM_EXTEND_ID ;
   typedef _utilMap<QGM_EXTEND_ID, qgmOptiTreeNode*, 8 > QGM_EXTEND_TABLE ;

   class _qgmExtendPlan : public SDBObject
   {
   public:
      _qgmExtendPlan() ;
      virtual ~_qgmExtendPlan() ;

   public:
      INT32 extend( qgmOptiTreeNode *&extended ) ;

      /// when local is not NULL, it will be defined as local.
      INT32 insertPlan( UINT32 id, qgmOptiTreeNode *ex = NULL ) ;

      OSS_INLINE qgmOptiTreeNode *getNode( UINT32 id )
      {
         QGM_EXTEND_TABLE::iterator it = _table.find( id ) ;
         return ( it == _table.end() ) ? NULL : it->second ;
      }

      OSS_INLINE void pushAlias( const qgmField &alias )
      {
         _aliases.push( alias ) ;
      }

   private:
      virtual INT32 _extend( UINT32 id,
                             qgmOptiTreeNode *&extended ) = 0 ;

   protected:
      QGM_EXTEND_TABLE _table ;
      qgmOptiTreeNode  *_local ;
      UINT32 _localID ;
      std::queue<qgmField> _aliases ;

   } ;

   typedef class _qgmExtendPlan qgmExtendPlan ;
}

#endif

