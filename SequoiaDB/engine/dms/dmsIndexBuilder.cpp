/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = dmsIndexBuilder.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/6/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsIndexBuilder.hpp"
#include "dmsStorageIndex.hpp"
#include "dmsStorageData.hpp"
#include "dmsIndexBuilderImpl.hpp"
#include "ixm.hpp"

namespace engine
{
   _dmsIndexBuilder::_dmsIndexBuilder( _dmsStorageIndex* indexSU,
                                       _dmsStorageData* dataSU,
                                       _dmsMBContext* mbContext,
                                       _pmdEDUCB* eduCB,
                                       dmsExtentID indexExtentID )
   : _suIndex ( indexSU ),
     _suData ( dataSU ),
     _mbContext ( mbContext ),
     _eduCB ( eduCB ),
     _indexExtentID ( indexExtentID )
   {
      _indexCB = NULL ;
      _scanExtLID = DMS_INVALID_EXTENT ;
      _currentExtentID = DMS_INVALID_EXTENT ;
      _extent = NULL ;
      _unique = FALSE ;
      _dropDups = FALSE ;
   }

   _dmsIndexBuilder::~_dmsIndexBuilder()
   {
      _suIndex = NULL ;
      _suData = NULL ;
      _mbContext = NULL ;
      if ( NULL != _indexCB )
      {
         SDB_OSS_DEL( _indexCB ) ;
         _indexCB = NULL ;
      }
   }

   INT32 _dmsIndexBuilder::_init()
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( _suIndex != NULL, "_suIndex can't be NULL" ) ;
      SDB_ASSERT( _suData != NULL, "_suData can't be NULL" ) ;
      SDB_ASSERT( _mbContext != NULL, "_mbContext can't be NULL" ) ;

      // lock mb
      rc = _mbContext->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      // make sure the extent is valid
      PD_CHECK ( DMS_INVALID_EXTENT != _indexExtentID,
                 SDB_INVALIDARG, error, PDERROR,
                 "Index Extent ID %d is invalid for collection %d",
                 _indexExtentID, (INT32)_mbContext->mbID() ) ;

      _indexCB = SDB_OSS_NEW ixmIndexCB ( _indexExtentID,
                                          _suIndex, _mbContext ) ;
      if ( NULL == _indexCB )
      {
         PD_LOG ( PDERROR, "failed to allocate _indexCB" ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      // verify the index control block is initialized
      PD_CHECK ( _indexCB->isInitialized(), SDB_DMS_INIT_INDEX,
                 error, PDERROR, "Failed to initialize index" ) ;

      _indexLID = _indexCB->getLogicalID() ;

      rc = _indexCB->getIndexID ( _indexOID ) ;
      PD_RC_CHECK ( rc, PDERROR, "Failed to get indexID, rc = %d", rc ) ;

      // already creating, continue
      if ( IXM_INDEX_FLAG_CREATING == _indexCB->getFlag() )
      {
         _scanExtLID = _indexCB->scanExtLID() ;
      }
      else if ( IXM_INDEX_FLAG_NORMAL != _indexCB->getFlag() &&
                IXM_INDEX_FLAG_INVALID != _indexCB->getFlag() )
      {
         PD_LOG ( PDERROR, "Index is either creating or dropping: %d",
                  (INT32)_indexCB->getFlag() ) ;
         _indexCB->setFlag ( IXM_INDEX_FLAG_INVALID ) ;
         rc = SDB_IXM_UNEXPECTED_STATUS ;
         goto error ;
      }
      else
      {
         // truncate index, do NOT remove root
         rc = _indexCB->truncate ( FALSE ) ;
         if ( rc )
         {
            PD_LOG ( PDERROR, "Failed to truncate index, rc: %d", rc ) ;
            _indexCB->setFlag ( IXM_INDEX_FLAG_INVALID ) ;
            goto error ;
         }
         _indexCB->setFlag ( IXM_INDEX_FLAG_CREATING ) ;
      }

      _unique = _indexCB->unique() ;
      _dropDups = _indexCB->dropDups() ;

      /// start rebuilding
      _currentExtentID = _mbContext->mb()->_firstExtentID ;
      if ( DMS_INVALID_EXTENT == _currentExtentID )
      {
         /// when the collection is empty, we complete the index creating
         /// fast( when unlock the context and scan the data, because the 
         /// scanExtLID always is -1, so when scan finished, the new instor
         /// will not insert to the index )
         _indexCB->setFlag ( IXM_INDEX_FLAG_NORMAL ) ;
         _indexCB->scanExtLID ( DMS_INVALID_EXTENT ) ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

   done:
      _mbContext->mbUnlock() ;
      return rc ;
   error:
      if ( NULL != _indexCB )
      {
         SDB_OSS_DEL( _indexCB ) ;
         _indexCB = NULL ;
      }
      goto done ;
   }

   INT32 _dmsIndexBuilder::_finish()
   {
      INT32 rc = SDB_OK ;

      if ( SDB_OK == ( rc = _checkIndexAfterLock( SHARED ) ) )
      {
         _indexCB->setFlag ( IXM_INDEX_FLAG_NORMAL ) ;
         _indexCB->scanExtLID ( DMS_INVALID_EXTENT ) ;
         _mbContext->mbUnlock() ;
      }

      return rc ;
   }

   INT32 _dmsIndexBuilder::build()
   {
      INT32 rc = SDB_OK ;

      rc = _init() ;
      if ( SDB_DMS_EOC == rc )
      {
         /// no data, has already finished
         rc = SDB_OK ;
         goto done ;
      }
      else if ( SDB_OK != rc )
      {
         goto init_error ;
      }

      rc = _build() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = _finish() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;
   init_error:
      goto done ;
   error:
      // do NOT set flag when SDB_DMS_INVALID_INDEXCB,
      // because something else may changed the indexCB,
      // it's possible other threads dropped index so that
      // the extent is reused. So we should NOT touch the block here
      if ( SDB_DMS_INVALID_INDEXCB != rc )
      {
         if ( SDB_OK == _checkIndexAfterLock( SHARED ) )
         {
            _indexCB->setFlag ( IXM_INDEX_FLAG_INVALID ) ;
            _mbContext->mbUnlock() ;
         }
      }
      goto done ;
   }

   INT32 _dmsIndexBuilder::_beforeExtent()
   {
      INT32 rc = SDB_OK ;

      // make sure the index cb is still valid
      if ( !_indexCB->isStillValid ( _indexOID ) ||
           _indexLID != _indexCB->getLogicalID() )
      {
         rc = SDB_DMS_INVALID_INDEXCB ;
         goto error ;
      }

      // get the address of extent indicated in context for where
      // should we starts
      _extent = (dmsExtent*)_suData->extentAddr ( _currentExtentID ) ;
      if ( NULL == _extent )
      {
         PD_LOG ( PDERROR, "Invalid extent: %d", _currentExtentID ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // find the extent by logical id
      if ( DMS_INVALID_EXTENT != _scanExtLID &&
           _extent->_logicID < _scanExtLID )
      {
         _currentExtentID = _extent->_nextExtent ;
         rc = _DMS_SKIP_EXTENT ;
      }

   done:
      return rc ;
   error:
      _extent = NULL ;
      goto done ;
   }

   INT32 _dmsIndexBuilder::_afterExtent()
   {
      if ( DMS_INVALID_EXTENT == _extent->_nextExtent )
      {
         // done scan, set scanned extent to maximum value
         // so coming write operators will update this index
         _indexCB->scanExtLID( DMS_MAX_SCANNED_EXTENT ) ;
      }
      else
      {
         _indexCB->scanExtLID ( _extent->_logicID ) ;
      }
      return SDB_OK ;
   }

   INT32 _dmsIndexBuilder::_getKeySet( ossValuePtr recordDataPtr, BSONObjSet& keySet )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObj obj ( (CHAR*)recordDataPtr ) ;

         rc = _indexCB->getKeysFromObject ( obj, keySet ) ;
         PD_RC_CHECK ( rc, PDERROR, "Failed to get keys from object %s",
                       obj.toString().c_str() ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG ( PDERROR, "Failed to create BSON object: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsIndexBuilder::_insertKey( const ixmKey &key, const dmsRecordID &rid, const Ordering& ordering )
   {
      INT32 rc = SDB_OK ;

      rc = _suIndex->_indexInsert( _indexCB, key, rid, ordering, _eduCB, !_unique, _dropDups ) ;
      if ( SDB_OK != rc )
      {
         // during index rebuild, it's possible some other
         // sessions inserted records that already stored in
         // index, and then when we scan the disk we read it
         // again. In  this case let's simply skip it
         if ( SDB_IXM_IDENTICAL_KEY == rc )
         {
            rc = SDB_OK ;
            PD_LOG ( PDWARNING, "Identical key is detected "
                     "during index rebuild, ignore" ) ;
         }
         else
         {
            // for any other index insert error, let's return error
            PD_LOG ( PDERROR, "Failed to insert into index, rc: %d", rc ) ;
         }
      }

      return rc ;
   }

   INT32 _dmsIndexBuilder::_insertKey( ossValuePtr recordDataPtr, const dmsRecordID &rid, const Ordering& ordering )
   {
      BSONObjSet keySet ;
      BSONObjSet::iterator it ;
      INT32 rc = SDB_OK ;

      rc = _getKeySet( recordDataPtr, keySet ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      for ( it = keySet.begin() ; it != keySet.end() ; it++ )
      {
         ixmKeyOwned ko( *it ) ;
         rc = _insertKey( ko, rid, ordering ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsIndexBuilder::_checkIndexAfterLock( INT32 lockType )
   {
      INT32 rc = SDB_OK ;

      rc = _mbContext->mbLock( lockType ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
         goto error ;
      }

      // make sure the index cb is still valid
      if ( !_indexCB->isStillValid ( _indexOID ) ||
           _indexLID != _indexCB->getLogicalID() )
      {
         rc = SDB_DMS_INVALID_INDEXCB ;
         goto error ;
      }

   done:
      return rc ;
   error:
      _mbContext->mbUnlock() ;
      goto done ;
   }

   _dmsIndexBuilder* _dmsIndexBuilder::createInstance( _dmsStorageIndex* indexSU,
                                                       _dmsStorageData* dataSU,
                                                       _dmsMBContext* mbContext,
                                                       _pmdEDUCB* eduCB,
                                                       dmsExtentID indexExtentID, 
                                                       INT32 sortBufferSize )
   {
      _dmsIndexBuilder* builder = NULL ;

      SDB_ASSERT( indexSU != NULL, "indexSU can't be NULL" ) ;
      SDB_ASSERT( dataSU != NULL, "dataSU can't be NULL" ) ;
      SDB_ASSERT( mbContext != NULL, "mbContext can't be NULL" ) ;
      SDB_ASSERT( eduCB != NULL, "eduCB can't be NULL" ) ;

      PD_LOG ( PDDEBUG, "index sort buffer size: %dMB", sortBufferSize ) ;

      if ( sortBufferSize < 0 )
      {
         PD_LOG ( PDERROR, "invalid sort buffer size: %d", sortBufferSize ) ;
      }
      else if ( 0 == sortBufferSize )
      {
         builder = SDB_OSS_NEW _dmsIndexOnlineBuilder( indexSU,
                                                       dataSU,
                                                       mbContext,
                                                       eduCB,
                                                       indexExtentID ) ;
         if ( NULL == builder)
         {
            PD_LOG ( PDERROR, "failed to allocate _dmsIndexOnlineBuilder" ) ;
         }
      }
      else
      {
         builder = SDB_OSS_NEW _dmsIndexSortingBuilder( indexSU,
                                                        dataSU,
                                                        mbContext,
                                                        eduCB,
                                                        indexExtentID,
                                                        sortBufferSize ) ;
         if ( NULL == builder)
         {
            PD_LOG ( PDERROR, "failed to allocate _dmsIndexSortingBuilder" ) ;
         }
      }

      return builder ;
   }

   void _dmsIndexBuilder::releaseInstance( _dmsIndexBuilder* builder )
   {
      SDB_ASSERT( builder != NULL, "builder can't be NULL" ) ;

      SDB_OSS_DEL builder ;
   }
}

