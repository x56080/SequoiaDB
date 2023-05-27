/*******************************************************************************


   Copyright (C) 2011-present SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = aggrUnwind.cpp

   Descriptive Name = Parser for $unwind in aggregation.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          20/05/2023  YSD  Initial Draft

   Last Changed =

******************************************************************************/
#include "aggrUnwind.hpp"
#include "aggrDef.hpp"
#include "qgmOptiSelect.hpp"
#include "msgDef.hpp"
#include "pdTrace.hpp"

namespace engine
{
   INT32 aggrUnwindParser::buildNode( const BSONElement &elem,
                                      const CHAR *pCLName,
                                      BSONObj &hint,
                                      qgmOptiTreeNode *&pNode,
                                      _qgmPtrTable *pTable,
                                      _qgmParamTable *pParamTable )
   {
      // $unwind is implemented based on 'SPLIT BY' clause in SQL.
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;

      qgmOptiSelect *pSelect = SDB_OSS_NEW qgmOptiSelect( pTable, pParamTable ) ;
      PD_CHECK( pSelect != NULL, SDB_OOM, error, PDERROR, "Allocate memory of size[%u] failed, "
                "rc: %d", sizeof( qgmOptiSelect ), rc ) ;

      try
      {
         if ( String == elem.type() )
         {
            rc = _parseUnwindFieldPath( elem.valuestr(), pSelect->_splitby, pTable, pCLName ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse the $unwind path[%s], rc: %d",
                         elem.toString( TRUE, TRUE ).c_str(), rc ) ;
         }
         else if ( Object == elem.type() )
         {
            rc = _parseUnWindOption( elem.embeddedObject(), pSelect->_splitby,
                                     pTable, pCLName, builder ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse the $unwind option[%s], rc: %d",
                         elem.toString( TRUE, TRUE ).c_str(), rc ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Expected either a string or an object as specification for "
                        "$unwind: %s, rc: %d", elem.toString( TRUE, TRUE ).c_str(), rc ) ;
            goto error ;
         }

         {
            qgmOpField selectAll ;
            selectAll.type = SQL_GRAMMAR::WILDCARD ;
            pSelect->_selector.push_back( selectAll ) ;
         }

         if ( !builder.isEmpty() )
         {
            builder.appendElements( hint ) ;
            pSelect->_objHint = builder.obj() ;
         }
         else
         {
            pSelect->_objHint = hint ;
         }

         aggrEmptyBSONObj( hint ) ;
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

      pSelect->_limit = -1 ;
      pSelect->_skip = 0 ;
      pSelect->_type = QGM_OPTI_TYPE_SELECT ;
      pSelect->_hasFunc = FALSE ;

      // No alias in aggregation, so use the default.
      rc = pTable->getOwnField( AGGR_CL_DEFAULT_ALIAS, pSelect->_alias ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get the own field[%s], rc: %d",
                   AGGR_CL_DEFAULT_ALIAS, rc ) ;
      if ( pCLName )
      {
         qgmField clValAttr ;
         qgmField clValRelegation ;
         rc = pTable->getOwnField( pCLName, clValAttr ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get the own field[%s], rc: %d",
                      pCLName, rc ) ;

         rc = pTable->getOwnField( AGGR_CL_DEFAULT_ALIAS,
                                   pSelect->_collection.alias ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get the own field[%s], rc: %d",
                      AGGR_CL_DEFAULT_ALIAS, rc ) ;
         pSelect->_collection.value = qgmDbAttr( clValRelegation, clValAttr ) ;
         pSelect->_collection.type = SQL_GRAMMAR::DBATTR ;
      }

      pNode = pSelect ;

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( pSelect ) ;
      goto done ;
   }

   INT32 aggrUnwindParser::_parseUnwindFieldPath( const CHAR *pFieldPath,
                                                  qgmDbAttr &splitby,
                                                  _qgmPtrTable *pTable,
                                                  const CHAR *pCLName )
   {
      INT32 rc = SDB_OK ;
      qgmField unwindValAttr ;
      qgmField unwindValRelegation ;
      const CHAR *pFieldName = NULL ;

      SDB_ASSERT( pFieldPath, "Field path is null" ) ;
      if ( ossStrlen( pFieldPath ) < 2 || '$' != pFieldPath[0] )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "Path option to $unwind should be prefixed with a '$': [%s], rc: %d",
                     pFieldPath, rc ) ;
         goto error ;
      }

      pFieldName = &pFieldPath[1] ;

      rc = pTable->getOwnField( AGGR_CL_DEFAULT_ALIAS, unwindValRelegation ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get the own field[%s], rc: %d",
                   AGGR_CL_DEFAULT_ALIAS, rc ) ;

      rc = pTable->getOwnField( pFieldName, unwindValAttr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get the own field[%s], rc: %d",
                   pFieldName, rc ) ;

      splitby = qgmDbAttr( unwindValRelegation, unwindValAttr ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 aggrUnwindParser::_parseUnWindOption( const BSONObj &option, qgmDbAttr &splitby,
                                               _qgmPtrTable *pTable, const CHAR *pCLName,
                                               BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN hasPath = FALSE ;

      try
      {
         BSONObjIterator it( option ) ;
         while ( it.more() )
         {
            BSONElement elem = it.next() ;
            if ( 0 == ossStrcasecmp( elem.fieldName(), FIELD_NAME_PATH ) )
            {
               if ( String == elem.type() )
               {
                  rc = _parseUnwindFieldPath( elem.valuestr(), splitby, pTable, pCLName ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to parse the $unwind field path[%s], rc: %d",
                               elem.toString( TRUE, TRUE ).c_str(), rc ) ;
                  hasPath = TRUE ;
               }
               else
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "Expected a string as the path for $unwind: %s, rc: %d",
                              elem.toString( TRUE, TRUE ).c_str(), rc ) ;
                  goto error ;
               }
            }
            else if ( 0 == ossStrcasecmp( elem.fieldName(),
                                          FIELD_NAME_STRICT ) )
            {
               if ( Bool == elem.type() )
               {
                  builder.append( elem ) ;
               }
               else
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "Expect a boolean for the strict option to $unwind: %s, "
                              "rc: %d", elem.toString( TRUE, TRUE ).c_str(), rc ) ;
                  goto error ;
               }
            }
            else if ( 0 == ossStrcasecmp( elem.fieldName(), FIELD_NAME_ARRAY_INDEX_ALIAS ) )
            {
               if ( String == elem.type() )
               {
                  builder.append( elem ) ;
               }
               else
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "Expected a non-empty string for the ArrayIndexAlias option "
                              "to $unwind: %s, rc: %d", elem.toString( TRUE, TRUE ).c_str(), rc ) ;
                  goto error ;
               }
            }
            else
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Unrecognized option to $unwind: %s, rc: %d",
                           elem.toString( TRUE, TRUE ).c_str(), rc ) ;
               goto error ;
            }
         }
      }
      catch ( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
         goto error ;
      }

      if ( !hasPath )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "No path specified to $unwind, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}
