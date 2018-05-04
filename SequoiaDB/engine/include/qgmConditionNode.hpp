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

   Source File Name = qgmConditionNode.hpp

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

#ifndef QGMCONDITIONNODE_HPP_
#define QGMCONDITIONNODE_HPP_

#include "qgmOptiTree.hpp"
#include "pd.hpp"

namespace engine
{
   struct _qgmConditionNode : public SDBObject
   {
      qgmDbAttr value ;

      /// when type > SQL_GRAMMAR::SQLMAX, var is effective.
      const BSONElement *var ;
      INT32 type ;
      _qgmConditionNode *left ;
      _qgmConditionNode *right ;

      _qgmConditionNode( INT32 t )
      :var(NULL),
       type( t ),
       left( NULL ),
       right( NULL )
      {

      }

      _qgmConditionNode( const _qgmConditionNode *node )
      {
         SDB_ASSERT( NULL != node, "impossible" ) ;
         type = node->type ;
         value = node->value ;
         var = node->var ;
         left = NULL ;
         right = NULL ;
         if ( NULL != node->left )
         {
            left = SDB_OSS_NEW _qgmConditionNode(node->left) ;
            if ( NULL == left )
            {
               PD_LOG( PDERROR, "failed to allocate mem." ) ;
            }
         }

         if ( NULL != node->right )
         {
            right = SDB_OSS_NEW _qgmConditionNode(node->right) ;
            if ( NULL == right )
            {
               PD_LOG( PDERROR, "failed to allocate mem." ) ;
            }
         }
      }

      virtual ~_qgmConditionNode()
      {
         SAFE_OSS_DELETE( left ) ;
         SAFE_OSS_DELETE( right ) ;
      }

      void  dettach ()
      {
         left = NULL ;
         right = NULL ;
      }
   } ;
   typedef struct _qgmConditionNode qgmConditionNode ;
   typedef vector< qgmConditionNode* > qgmConditionNodePtrVec ;
   typedef vector< qgmConditionNode >  qgmConditionNodeVec ;
}

#endif

