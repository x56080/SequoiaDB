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

   Source File Name = rtnIxmKeySorter.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/6/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnIxmKeySorter.hpp"
#include <algorithm>
#include "ossUtil.h"

namespace engine
{
   _rtnIxmKeySorter::_rtnIxmKeySorter( INT64 bufSize, const _dmsIxmKeyComparer& comparer )
   : _dmsIxmKeySorter( bufSize, comparer )
   {
      _buf = NULL ;
      _headOffset = 0 ;
      _tailOffset = bufSize ;
      _keyNum = 0 ;
      _fetchedNum = 0 ;
      _sorted = FALSE ;
      _inited = FALSE ;
   }

   _rtnIxmKeySorter::~_rtnIxmKeySorter()
   {
      if ( NULL != _buf )
      {
         SDB_OSS_FREE( _buf ) ;
         _buf = NULL ;
      }
   }

   INT32 _rtnIxmKeySorter::init()
   {
      INT32 rc = SDB_OK ;

      if ( !_inited )
      {
         _buf = ( CHAR* )SDB_OSS_MALLOC( _bufSize ) ;
         if ( NULL == _buf )
         {
            PD_LOG( PDERROR, "failed to allocate buffer for index sorting." ) ;
            rc = SDB_OOM ;
            goto error ;
         }

         _inited = TRUE ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
   
   INT32 _rtnIxmKeySorter::reset()
   {
      SDB_ASSERT( _inited, "must be inited" ) ;
      _headOffset = 0 ;
      _tailOffset = _bufSize ;
      _keyNum = 0 ;
      _fetchedNum = 0 ;
      _sorted = FALSE ;
      return SDB_OK ;
   }

   INT32 _rtnIxmKeySorter::push( const ixmKey& key, const dmsRecordID& recordID )
   {
      INT32 keyDataSize = key.dataSize() ;
      INT32 rc = SDB_OK ;
      CHAR* keyPosition ;
      dmsRecordID* rid ;
      CHAR** keySlot ;

      SDB_ASSERT( _inited, "must be inited before pushing" ) ;
      SDB_ASSERT( !_sorted, "already sorted, can't push" ) ;

      if ( (INT64)( keyDataSize + sizeof(dmsRecordID) + sizeof(ixmKey*) ) >
           _tailOffset - _headOffset )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      _tailOffset -= keyDataSize + sizeof(dmsRecordID) ;

      // copy key data
      keyPosition = _buf + _tailOffset ;
      ossMemcpy( keyPosition , key.data(), keyDataSize ) ;

      //copy record id
      rid = (dmsRecordID*)( keyPosition + keyDataSize ) ;
      *rid = recordID ;

      // set key slot
      keySlot = (CHAR**)( _buf + _headOffset ) ;
      *keySlot = keyPosition ;
      _headOffset += sizeof(ixmKey*) ;

      _keyNum++ ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnIxmKeySorter::sort()
   {
      SDB_ASSERT( _inited, "must be inited" ) ;
      SDB_ASSERT( !_sorted, "already sorted") ;

      if ( _keyNum > 0 )
      {
         CHAR** keyStart = (CHAR**)_buf ;
         CHAR** keyEnd = (CHAR**)( _buf + _keyNum * sizeof(ixmKey*) ) ;
         std::sort( keyStart, keyEnd, _comparer ) ;
      }

      _sorted = TRUE ;
      return SDB_OK ;
   }

   INT32 _rtnIxmKeySorter::fetch( ixmKey& key, dmsRecordID& recordID )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( _inited, "must be inited" ) ;

      if( _fetchedNum < _keyNum )
      {
         CHAR* keyData = *(CHAR**)( _buf + sizeof(ixmKey*) * _fetchedNum ) ;
         key.assign( ixmKey( keyData ) );
         recordID = *(dmsRecordID*)( keyData + key.dataSize() ) ;
         _fetchedNum++ ;
      }
      else
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT64 _rtnIxmKeySorter::usedBufferSize() const
   {
      return ( _bufSize - _tailOffset + _headOffset ) ;
   }

   _dmsIxmKeySorter* _rtnIxmKeySorterCreator::createSorter(  INT64 bufSize, const _dmsIxmKeyComparer& comparer )
   {
      INT32 rc = SDB_OK ;
      _rtnIxmKeySorter* sorter = NULL ;

      sorter = SDB_OSS_NEW _rtnIxmKeySorter( bufSize, comparer ) ;
      if ( NULL == sorter )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "failed to create _rtnIxmKeySorter, rc: %d", rc ) ;
         goto error ;
      }

      rc = sorter->init() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to init _rtnIxmKeySorter, rc: %d", rc ) ;
         goto error ;
      }

   done:
      return sorter ;
   error:
      if ( NULL != sorter )
      {
         SDB_OSS_DEL( sorter ) ;
         sorter = NULL ;
      }
      goto done ;
   }

   void _rtnIxmKeySorterCreator::releaseSorter( _dmsIxmKeySorter* sorter )
   {
      SDB_ASSERT( NULL != sorter, "sorter can't be NULL" ) ;

      SDB_OSS_DEL( sorter ) ;
   }
}

