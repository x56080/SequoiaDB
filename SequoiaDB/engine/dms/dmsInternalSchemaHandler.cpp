#include "dmsInternalSchemaHandler.hpp"
#include "dmsInternalSchema.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsInternalSchemaUpdator.hpp"

namespace engine
{

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
      PD_CHECK( holder, SDB_SYS, error, PDERROR, "Failed to get dms event holder in callback of "
                "internal schema when creating index, rc: %d", rc ) ;

      su = holder->getSU() ;
      PD_CHECK( su, SDB_SYS, error, PDERROR, "Failed to get storage unit from event holder, rc: %d",
                rc ) ;

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
