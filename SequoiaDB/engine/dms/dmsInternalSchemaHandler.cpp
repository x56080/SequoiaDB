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
         // Loop the columns in the index definition, and mark them in the internal schema.
         BSONObj keyPattern = idxItem._boDefine.getObjectField( IXM_KEY_FIELD ) ;
         BSONObjIterator itr( keyPattern ) ;

         dmsInternalSchemaWriter schemaUpdator ;

         dmsExtRW schemaExtRW = su->data()->extent2RW( context->mb()->_schemaExtentID ) ;
         dmsExtRW hashExtRW = su->data()->extent2RW( context->mb()->_schemaHashExtentID ) ;
         schemaExtRW.setNothrow( TRUE ) ;
         hashExtRW.setNothrow( TRUE ) ;
         dmsSchemaExtent *schemaExtent =
            schemaExtRW.writePtr<dmsSchemaExtent>(0, schema->getSchemaContainer()->getExtentSize() ) ;
         dmsSchemaHashExtent *hashExtent =
            hashExtRW.writePtr<dmsSchemaHashExtent>(0, schema->getSchemaHashTable()->getExtentSize() ) ;
         rc = schemaUpdator.init( su->data(), context, schemaExtent, hashExtent ) ;
         PD_RC_CHECK( rc, PDERROR, "Init internal schema of collection[%s] failed, rc: %d",
                      context->mb()->_collectionName, rc ) ;

         while ( itr.more() )
         {
            BSONElement ele = itr.next() ;
            rc = schemaUpdator.setIndexColumn( ele.fieldName() ) ;
            PD_RC_CHECK( rc, PDERROR, "Set column %s as index column in internal schema failed, "
                         "rc: %d", ele.fieldName(), rc ) ;
         }

         rc = schemaUpdator.save( context ) ;
         PD_RC_CHECK( rc, PDERROR, "Save new internal schema of collection[%s] failed, rc: %d",
                      context->mb()->_collectionName, rc ) ;

         //rc = schema->reload() ;
         PD_RC_CHECK( rc, PDERROR, "Reload new internal schema of collection[%s] failed, rc: %d",
                      rc ) ;
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
      PD_CHECK( holder, SDB_SYS, error, PDERROR, "Failed to get dms event holder in callback of "
                "internal schema when creating index, rc: %d", rc ) ;

      su = holder->getSU() ;
      PD_CHECK( su, SDB_SYS, error, PDERROR, "Failed to get storage unit from event holder, rc: %d",
                rc ) ;

      schema = su->data()->getSchema( clItem._mbID ) ;
      if ( schema->enabled() )
      {
         // For each column in the current index, we need to check if it still exists in other
         // indexes. If yes, we should not remove the index column mark.
         dmsInternalSchemaWriter schemaUpdator ;
         dmsExtRW schemaExtRW = su->data()->extent2RW( context->mb()->_schemaExtentID ) ;
         dmsExtRW hashExtRW = su->data()->extent2RW( context->mb()->_schemaHashExtentID ) ;
         schemaExtRW.setNothrow( TRUE ) ;
         hashExtRW.setNothrow( TRUE ) ;
         dmsSchemaExtent *schemaExtent =
            schemaExtRW.writePtr<dmsSchemaExtent>(0, schema->getSchemaContainer()->getExtentSize() ) ;
         dmsSchemaHashExtent *hashExtent =
            schemaExtRW.writePtr<dmsSchemaHashExtent>(0, schema->getSchemaHashTable()->getExtentSize() ) ;
         rc = schemaUpdator.init( su->data(), context, schemaExtent, hashExtent ) ;
         PD_RC_CHECK( rc, PDERROR, "Init internal schema of collection[%s] failed, rc: %d",
                      context->mb()->_collectionName, rc ) ;
         try
         {
            BSONObj keyPattern = idxItem._boDefine.getObjectField( IXM_KEY_FIELD ) ;
            BSONObjIterator currIdxItr( keyPattern ) ;
            dmsMBContext *context = clItem._mbContext ;
            ossPoolSet<const CHAR *, cmp_column_name> idxColumnNames ;
            for ( UINT32 id = 0; id < context->mb()->_numIndexes; ++id )
            {
               ixmIndexCB indexCB( context->mb()->_indexExtent[id], su->index(), context ) ;
               PD_CHECK( indexCB.isInitialized(), SDB_DMS_INIT_INDEX, error, PDERROR,
                         "Failed to initialize index" ) ;
               // Skip the current index to be dropped.
               if ( idxItem._idxLID != indexCB.getLogicalID() )
               {
                  BSONObj keyPattern = indexCB.keyPattern() ;
                  BSONObjIterator itr( keyPattern ) ;
                  while ( itr.more() )
                  {
                     BSONElement ele = itr.next() ;
                     idxColumnNames.insert( ele.fieldName() ) ;
                  }
               }
            }

            while ( currIdxItr.more() )
            {
               BSONElement ele = currIdxItr.next() ;
               if ( idxColumnNames.end() == idxColumnNames.find( ele.fieldName() ) )
               {
                  rc = schemaUpdator.unsetIndexColumn( ele.fieldName() ) ;
                  PD_RC_CHECK( rc, PDERROR, "Unset index column flag for column[%s] failed, rc: %d",
                               ele.fieldName(), rc ) ;
               }
            }

            rc = schemaUpdator.save( context ) ;
            PD_RC_CHECK( rc, PDERROR, "Save new internal schema of collection[%s] failed, rc: %d",
                         context->mb()->_collectionName, rc ) ;

            //rc = schema->reload() ;
            PD_RC_CHECK( rc, PDERROR, "Reload new internal schema of collection[%s] failed, rc: %d",
                         rc ) ;
         }
         catch ( std::exception &e )
         {
            rc = ossException2RC( &e ) ;
            PD_LOG( PDERROR, "Unexpected exception occurred: %s", e.what() ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}
