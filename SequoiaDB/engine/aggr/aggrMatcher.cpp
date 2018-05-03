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

   Source File Name = aggrMatcher.cpp

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
#include "aggrMatcher.hpp"
#include "qgmOptiMthMatchSelect.hpp"
#include "aggrDef.hpp"

using namespace bson ;

namespace engine
{
   /*
      aggrMatchParser implement
   */
   INT32 aggrMatchParser::buildNode( const BSONElement &elem,
                                     const CHAR *pCLName,
                                     qgmOptiTreeNode *&pNode,
                                     _qgmPtrTable *pTable,
                                     _qgmParamTable *pParamTable )
   {
      INT32 rc = SDB_OK ;
      qgmOptiMthMatchSelect *pSelect = NULL ;

      pSelect = SDB_OSS_NEW qgmOptiMthMatchSelect( pTable,
                                                   pParamTable ) ;
      PD_CHECK( pSelect != NULL, SDB_OOM, error, PDERROR,
                "Malloc failed!" ) ;

      try
      {
         BSONObj obj ;
         PD_CHECK( elem.type() == Object, SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse the parameter[%s], should object",
                   elem.toString( TRUE, TRUE ).c_str() ) ;

         obj = elem.embeddedObject() ;

         rc = pSelect->fromBson( obj ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build the node, rc: %d", rc ) ;
      }
      catch ( std::exception &e )
      {
         PD_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse the Parameter, occur unexpection: %s",
                   e.what() ) ;
      }

      {
         qgmOpField selectAll ;
         selectAll.type = SQL_GRAMMAR::WILDCARD ;
         pSelect->_selector.push_back( selectAll ) ;
         pSelect->_limit = -1 ;
         pSelect->_skip = 0 ;
         pSelect->_type = QGM_OPTI_TYPE_MTHMCHSEL ;
         pSelect->_hasFunc = FALSE ;
         rc = pTable->getOwnField( AGGR_CL_DEFAULT_ALIAS, pSelect->_alias ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get the own field[%s], rc: %d",
                      AGGR_CL_DEFAULT_ALIAS, rc ) ;

         if ( pCLName != NULL )
         {
            qgmField clValAttr ;
            qgmField clValRelegation ;
            rc = pTable->getOwnField( pCLName, clValAttr ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get the field[%s], rc: %d",
                         pCLName, rc ) ;
            rc = pTable->getOwnField( AGGR_CL_DEFAULT_ALIAS,
                                      pSelect->_collection.alias ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get the field[%s], rc: %d",
                         AGGR_CL_DEFAULT_ALIAS, rc ) ;

            pSelect->_collection.value = qgmDbAttr( clValRelegation,
                                                    clValAttr ) ;
            pSelect->_collection.type = SQL_GRAMMAR::DBATTR ;
         }
      }

      pNode = pSelect ;
   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pSelect ) ;
      goto done ;
   }

}

