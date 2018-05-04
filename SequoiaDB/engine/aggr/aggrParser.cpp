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

   Source File Name = aggrParser.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/04/2013  JHL  Initial Draft

   Last Changed =

******************************************************************************/
#include "aggrParser.hpp"
#include "qgmDef.hpp"
#include "qgmOptiSelect.hpp"
#include "aggrDef.hpp"
#include "msgDef.h"
#include "qgmOptiTree.hpp"

using namespace bson;

namespace engine
{
   /*
      aggrParser implement
   */
   INT32 aggrParser::parse( const BSONElement &elem,
                            _qgmOptiTreeNode *&root,
                            _qgmPtrTable * pPtrTable,
                            _qgmParamTable *pParamTable,
                            const CHAR *pCollectionName )
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT( pPtrTable!=NULL, "pPtrTable can't be NULL!" );
      SDB_ASSERT( pParamTable!=NULL, "pParamTable can't be NULL!" );
      SDB_ASSERT( pCollectionName != NULL || root != NULL,
                  "collectionname can't be NULL in leaf-node" );
      _qgmOptiTreeNode *pNode = NULL;

      rc = buildNode( elem, pCollectionName, pNode,
                      pPtrTable, pParamTable ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build the node, rc: %d", rc ) ;

      if ( root != NULL )
      {
         rc = pNode->addChild( root ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to add the child, rc: %d", rc ) ;
         root->_father = pNode ;
      }

      root = pNode ;

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pNode ) ;
      goto done ;
   }

}

