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

   Source File Name = dmsIndexBuilderImpl.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/6/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsIndexBuilderImpl.hpp"
#include "dmsStorageIndex.hpp"
#include "dmsStorageData.hpp"
#include "dmsScanner.hpp"
#include "ixmKey.hpp"
#include "ixm.hpp"
#include "dmsCB.hpp"
#include "pmdEDU.hpp"

namespace engine
{
   _dmsIndexOnlineBuilder::_dmsIndexOnlineBuilder( _dmsStorageIndex* indexSU,
                                                   _dmsStorageData* dataSU,
                                                   _dmsMBContext* mbContext,
                                                   _pmdEDUCB* eduCB,
                                                   dmsExtentID indexExtentID )
   : _dmsIndexBuilder( indexSU, dataSU, mbContext, eduCB, indexExtentID )
   {
   }

   _dmsIndexOnlineBuilder::~_dmsIndexOnlineBuilder()
   {
   }

   INT32 _dmsIndexOnlineBuilder::_build()
   {
      INT32 rc = SDB_OK ;
      Ordering ordering = Ordering::make( _indexCB->keyPattern() ) ;

      // loop through each extent
      while ( DMS_INVALID_EXTENT != _currentExtentID )
      {
         rc = _mbContext->mbLock( SHARED ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "dms mb context lock failed, rc: %d", rc ) ;
            goto error ;
         }

         rc = _beforeExtent() ;
         if ( SDB_OK != rc )
         {
            if ( _DMS_SKIP_EXTENT == rc )
            {
               _mbContext->mbUnlock() ;
               continue ;
            }

            goto error ;
         }

         dmsExtScanner extScanner( _suData, _mbContext, NULL, _currentExtentID ) ;
         _mthRecordGenerator generator ;
         dmsRecordID recordID ;
         ossValuePtr recordDataPtr ;

         while ( SDB_OK == ( rc = extScanner.advance( recordID, generator, _eduCB ) ) )
         {
            generator.getDataPtr( recordDataPtr ) ;
            rc = _insertKey( recordDataPtr, recordID, ordering ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }

         if ( SDB_DMS_EOC != rc )
         {
            PD_LOG ( PDERROR, "Failed to get record: %d", rc ) ;
            goto error ;
         }
         else
         {
            rc = SDB_OK ;
         }

         _currentExtentID = _extent->_nextExtent ;

         rc = _afterExtent() ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         _mbContext->mbUnlock() ;
      }

   done:
      _mbContext->mbUnlock() ;
      return rc ;
   error:
      goto done ;
   }

   _dmsIndexSortingBuilder::_dmsIndexSortingBuilder( _dmsStorageIndex* indexSU,
                                                     _dmsStorageData* dataSU,
                                                     _dmsMBContext* mbContext,
                                                     _pmdEDUCB* eduCB,
                                                     dmsExtentID indexExtentID,
                                                     INT32 sortBufferSize )
   : _dmsIndexBuilder( indexSU, dataSU, mbContext, eduCB, indexExtentID )
   {
      _sorter = NULL ;
      _eoc = FALSE ;
      _bufSize = (INT64)sortBufferSize * 1024 * 1024 ;

      // add extend size for fetching records by extent granularity
      // and to prevent sorter's buffer overflowing.
      // so we assign max extent size to ensure the sorter can't 
      // overflow when fetching records from a extent.
      _bufExtSize = DMS_MAX_EXTENT_SZ ;
   }

   _dmsIndexSortingBuilder::~_dmsIndexSortingBuilder()
   {
      if ( NULL != _sorter )
      {
         sdbGetDMSCB()->releaseIxmKeySorter( _sorter ) ;
         _sorter = NULL ;
      }
   }

   INT32 _dmsIndexSortingBuilder::_init()
   {
      INT32 rc = SDB_OK ;

      Ordering ordering = Ordering::make( _indexCB->keyPattern() ) ;
      _dmsIxmKeyComparer comparer( ordering ) ;

      _sorter = sdbGetDMSCB()->createIxmKeySorter( _bufSize + _bufExtSize, comparer ) ;
      if ( NULL == _sorter )
      {
         PD_LOG( PDERROR, "failed to create ixmKey sorter" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsIndexSortingBuilder::_fillSorter()
   {
      INT32 rc = SDB_OK ;

      for(;;)
      {
         if ( DMS_INVALID_EXTENT == _currentExtentID )
         {
            if ( _eoc )
            {
               rc = SDB_DMS_EOC ;
               goto error ;
            }
            else
            {
               _eoc = TRUE ;
               rc = SDB_OK ;
               goto done ;
            }
         }

         if ( _sorter->usedBufferSize() > _bufSize )
         {
            // the sorter's buffer is logical overflow (actually not overflow in sorter),
            // so stop fetching records from next extent
            PD_LOG( PDDEBUG, "sorter is full, bufSize=%lld, usedBufSize=%lld, total=%lld",
                    _bufSize, _sorter->usedBufferSize(), _sorter->bufferSize() ) ;
            goto done ;
         }

         rc = _beforeExtent() ;
         if ( SDB_OK != rc )
         {
            if ( _DMS_SKIP_EXTENT == rc )
            {
               continue ;
            }

            goto error ;
         }

         dmsExtScanner extScanner( _suData, _mbContext, NULL, _currentExtentID ) ;
         _mthRecordGenerator generator ;
         dmsRecordID recordID ;
         ossValuePtr recordDataPtr ;

         while ( SDB_OK == ( rc = extScanner.advance( recordID, generator, _eduCB ) ) )
         {
            BSONObjSet keySet ;
            BSONObjSet::iterator it ;
            generator.getDataPtr( recordDataPtr ) ;

            rc = _getKeySet( recordDataPtr, keySet ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }

            for ( it = keySet.begin() ; it != keySet.end() ; it++ )
            {
               ixmKeyOwned key( *(it) ) ;
               rc = _sorter->push( key, recordID ) ;
               if ( SDB_OK != rc )
               {
                  SDB_ASSERT( SDB_DMS_EOC != rc, "sorter can't overflow" ) ;
                  goto error ;
               }
            }
         }

         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_OK ;
         }
         else
         {
            goto error ;
         }

         _currentExtentID = _extent->_nextExtent ;

         rc = _afterExtent() ;
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


   INT32 _dmsIndexSortingBuilder::_insertKeys( const Ordering& ordering )
   {
      #define _KEYS_PER_BATCH 10000
      INT32 rc = SDB_OK ;

      for (;;)
      {
         ixmKey key ;
         dmsRecordID recordID ;

         if ( _eduCB->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         for ( INT32 i = 0 ; i < _KEYS_PER_BATCH ; i++ )
         {
            rc = _sorter->fetch( key, recordID ) ;
            if ( SDB_OK == rc )
            {
               rc = _insertKey( key, recordID, ordering ) ;
               if ( SDB_OK != rc )
               {
                  goto error ;
               }
            }
            else if ( SDB_DMS_EOC != rc )
            {
               goto error ;
            }
            else
            {
               rc = SDB_OK ;
               goto done ;
            }
         }

      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsIndexSortingBuilder::_build()
   {
      INT32 rc = SDB_OK ;

      Ordering ordering = Ordering::make( _indexCB->keyPattern() ) ;

      rc = _init() ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      for(;;)
      {
         if ( _eoc )
         {
            goto done ;
         }

         rc = _sorter->reset() ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         rc = _checkIndexAfterLock( SHARED ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         rc = _fillSorter() ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         rc = _sorter->sort() ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         rc = _insertKeys( ordering ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         _mbContext->mbUnlock() ;
      }

   done:
      _mbContext->mbUnlock() ;
      return rc ;
   error:
      goto done;
   }

}

