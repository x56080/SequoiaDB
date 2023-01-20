#include "dmsInternalSchemaHandler.hpp"
#include "dmsInternalSchema.hpp"
#include "dmsStorageUnit.hpp"

namespace engine
{
   // TODO: YSD enable infoschema 的时候也要对索引字段进行标记
   //       add schema too






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
         while ( itr.more() )
         {
            BSONElement ele = itr.next() ;
            rc = schema->setIndexColumn( clItem._mbContext, ele.fieldName() ) ;
            PD_RC_CHECK( rc, PDERROR, "Set column %s as index column in internal schema failed, "
                         "rc: %d", ele.fieldName(), rc ) ;
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
                  schema->unsetIndexColumn( context, ele.fieldName() ) ;
               }
            }
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
