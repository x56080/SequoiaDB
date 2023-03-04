/*******************************************************************************

   Copyright (C) 2011-2023 SequoiaDB Ltd.

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

   Source File Name = dmsInternalSchemaHandler.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/11/2023  YSD Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsInternalSchemaHandler.hpp"
#include "dmsInternalSchema.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsInternalSchemaUpdator.hpp"

using namespace bson ;

namespace engine
{

   /*
      _dmsInternalSchemaHandler implement
   */
   _dmsInternalSchemaHandler::_dmsInternalSchemaHandler()
   {
   }

   INT32 _dmsInternalSchemaHandler::onCreateIndex ( IDmsEventHolder *pEventHolder,
                                                    IDmsSUCacheHolder *pCacheHolder,
                                                    const dmsEventCLItem &clItem,
                                                    const dmsEventIdxItem &idxItem,
                                                    pmdEDUCB *cb,
                                                    SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      dmsStorageUnit *su = NULL ;
      dmsInternalSchema *schema = NULL ;
      dmsMBContext *context = clItem._mbContext ;

      dmsEventHolder *holder = dynamic_cast<dmsEventHolder *>( pEventHolder ) ;
      if ( !holder )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to get dms event holder in callback of "
                 "internal schema when creating index, rc: %d", rc ) ;
         goto error ;
      }

      su = holder->getSU() ;
      if ( !su )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to get storage unit from event holder, rc: %d", rc ) ;
         goto error ;
      }

      schema = su->data()->getSchema( clItem._mbID ) ;
      if ( schema->enabled() )
      {
         BOOLEAN hasChanged = FALSE ;
         dmsInternalSchemaWriter schemaUpdator ;
         ossPoolSet<ossPoolString> setFields ;

         rc = schemaUpdator.init( schema, su->data(), context, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Init schema updator failed, rc: %d", rc ) ;

         rc = su->index()->getIndexFields( context, setFields, idxItem._pIXName,
                                           idxItem._idxLID, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Get index fields failed, rc: %d", rc ) ;

         for ( ossPoolSet<ossPoolString>::iterator it = setFields.begin() ;
               it != setFields.end() ;
               ++it )
         {
            rc = schemaUpdator.setIndexColumn( (*it).c_str() ) ;
            PD_RC_CHECK( rc, PDERROR, "Set column %s as index column in internal schema "
                         "failed, rc: %d", (*it).c_str(), rc ) ;
         }

         rc = schemaUpdator.save( context, cb, hasChanged ) ;
         PD_RC_CHECK( rc, PDERROR, "Save new internal schema of collection[%s] failed, rc: %d",
                      context->mb()->_collectionName, rc ) ;

         if ( hasChanged )
         {
            rc = su->data()->reloadSchema( context, &schema ) ;
            PD_RC_CHECK( rc, PDERROR, "Reload internal schema of collection[%s] failed, rc: %d",
                         context->mb()->_collectionName, rc ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsInternalSchemaHandler::onDropIndex ( IDmsEventHolder *pEventHolder,
                                                  IDmsSUCacheHolder *pCacheHolder,
                                                  const dmsEventCLItem &clItem,
                                                  const dmsEventIdxItem &idxItem,
                                                  pmdEDUCB *cb,
                                                  SDB_DPSCB *dpsCB )
   {
      INT32 rc = SDB_OK ;
      dmsStorageUnit *su = NULL ;
      dmsInternalSchema *schema = NULL ;
      dmsMBContext *context = clItem._mbContext ;

      dmsEventHolder *holder = dynamic_cast<dmsEventHolder *>( pEventHolder ) ;
      if ( holder )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to get dms event holder in callback of "
                 "internal schema when creating index, rc: %d", rc ) ;
         goto error ;
      }

      su = holder->getSU() ;
      if ( !su )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to get storage unit from event holder, rc: %d", rc ) ;
         goto error ;
      }

      schema = su->data()->getSchema( clItem._mbID ) ;
      if ( schema->enabled() )
      {
         // For each column in the current index, we need to check if it still exists in other
         // indexes. If yes, we should not remove the index column mark.

         BOOLEAN hasChanged = FALSE ;
         dmsInternalSchemaWriter schemaUpdator ;
         ossPoolSet<ossPoolString> setFields ;
         ossPoolSet<ossPoolString> setFieldsDroped ;

         rc = schemaUpdator.init( schema, su->data(), context, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Init schema updator failed, rc: %d", rc ) ;

         /// get droped fields
         rc = su->index()->getIndexFields( context, setFieldsDroped, idxItem._pIXName,
                                           idxItem._idxLID, TRUE ) ;
         PD_RC_CHECK( rc, PDERROR, "Get index fields failed, rc: %d", rc ) ;

         if ( setFieldsDroped.empty() )
         {
            goto done ;
         }

         /// get other fields
         rc = su->index()->getIndexesFields( context, setFields, TRUE, idxItem._idxLID ) ;
         PD_RC_CHECK( rc, PDERROR, "Get all index fields failed, rc: %d", rc ) ;

         for ( ossPoolSet<ossPoolString>::iterator it = setFieldsDroped.begin() ;
               it != setFieldsDroped.end() ;
               ++it )
         {
            /// the field is not in other indexes
            if ( 0 == setFields.count( *it ) )
            {
               rc = schemaUpdator.unsetIndexColumn( (*it).c_str() ) ;
               PD_RC_CHECK( rc, PDERROR, "Unset column %s as index column in internal schema "
                            "failed, rc: %d", (*it).c_str(), rc ) ;
            }
         }

         rc = schemaUpdator.save( context, cb, hasChanged ) ;
         PD_RC_CHECK( rc, PDERROR, "Save new internal schema of collection[%s] failed, rc: %d",
                      context->mb()->_collectionName, rc ) ;

         if ( hasChanged )
         {
            rc = su->data()->reloadSchema( context, &schema ) ;
            PD_RC_CHECK( rc, PDERROR, "Reload internal schema of collection[%s] failed, rc: %d",
                         context->mb()->_collectionName, rc ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}
