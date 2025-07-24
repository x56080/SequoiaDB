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

   Source File Name = rtnLobWindow.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnLobWindow.hpp"
#include "rtnTrace.hpp"
#include "pdTrace.hpp"

namespace engine
{
const UINT32 RTN_MIN_READ_LEN = DMS_PAGE_SIZE512K ;
const UINT32 RTN_MAX_READ_LEN = DMS_PAGE_SIZE128K * 512 ;      /// 64MB

   _rtnLobWindow::_rtnLobWindow()
   :_pageSize( DMS_DO_NOT_CREATE_LOB ),
    _logarithmic( 0 ),
    _mergeMeta( FALSE ),
    _curOffset( 0 ),
    _pool( NULL ),
    _cachedSz( 0 ),
    _metaSize( 0 ),
    _analysisCache( FALSE )
   {
   }

   _rtnLobWindow::~_rtnLobWindow()
   {
      if ( NULL != _pool )
      {
         SDB_OSS_FREE( _pool ) ;
         _pool = NULL ; 
      }
   }

   UINT32 _rtnLobWindow::_getCurDataPageSize() const
   {
      if ( !_mergeMeta || _curOffset >= _pageSize - DMS_LOB_META_LENGTH )
      {
         return _pageSize ;
      }
      return _pageSize - DMS_LOB_META_LENGTH ;
   }

   UINT32 _rtnLobWindow::_getCurDataOffset() const
   {
      if ( !_mergeMeta || _curOffset >= _pageSize - DMS_LOB_META_LENGTH )
      {
         return 0 ;
      }
      return DMS_LOB_META_LENGTH ;
   }

   INT32 _rtnLobWindow::init( INT32 pageSize, BOOLEAN mergeMeta )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( DMS_DO_NOT_CREATE_LOB < pageSize,
                  "invalid arguments" ) ;

      SDB_ASSERT( _writeData.empty(), "impossible" ) ;

      if ( !ossIsPowerOf2( pageSize, &_logarithmic ) )
      {
         PD_LOG( PDERROR, "Invalid page size:%d, it should be a power of 2",
                 pageSize ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      /// first page for the last data
      /// second page for the meta data
      _pool = ( CHAR * )SDB_OSS_MALLOC( pageSize * 2 ) ;
      if ( NULL == _pool )
      {
         PD_LOG( PDERROR, "failed to allocate mem." ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      _pageSize = pageSize ;
      _mergeMeta = mergeMeta ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBWINDOW_ADDOUTPUTDATA, "_rtnLobWindow::addOutputData" )
   INT32 _rtnLobWindow::prepare2Write( SINT64 offset, UINT32 len,
                                       const CHAR *data )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBWINDOW_ADDOUTPUTDATA ) ;
      SDB_ASSERT( 0 <= offset && NULL != data, "invalid arguments" ) ;
      SDB_ASSERT( _writeData.empty(), "the last write has not been done" ) ;

      /// TOOD: seek write ?
      if ( offset != _curOffset + _cachedSz )
      {
         PD_LOG( PDERROR, "Invalid offset:%lld, current offset:%lld"
                 ", we do not support seek write yet",
                 offset, _curOffset ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      /// put the meta data to second page
      if ( _mergeMeta && _curOffset < _pageSize - DMS_LOB_META_LENGTH )
      {
         UINT32 lastLen = _pageSize - DMS_LOB_META_LENGTH - _curOffset ;
         CHAR *pCurMeta = _pool + _pageSize + DMS_LOB_META_LENGTH + _curOffset ;

         if ( len <= lastLen )
         {
            ossMemcpy( pCurMeta, data, len ) ;
            _metaSize += len ;
            _curOffset += len ;
            goto done ;
         }
         else
         {
            ossMemcpy( pCurMeta, data, lastLen ) ;
            _metaSize += lastLen ;
            SDB_ASSERT( _pageSize - DMS_LOB_META_LENGTH == _metaSize,
                        "meta size must be pagesize - metalen" ) ;
            _curOffset += lastLen ;
            SDB_ASSERT( _pageSize - DMS_LOB_META_LENGTH == _curOffset,
                        "Cur offset must be pageSize - metaLen" ) ;
            len -= lastLen ;
            data += lastLen ;
         }
      }

      /// never cached data
      if ( 0 == _cachedSz )
      {
         _writeData.tuple.columns.offset = offset ;
         _writeData.tuple.columns.len = len ;
         _writeData.data = data ;
         _analysisCache = FALSE ;
      }
      else
      {
         /// join cached data and write data.
         SDB_ASSERT( (UINT32)_cachedSz < _getCurDataPageSize(), "impossible" ) ;
         INT32 mvSize = _getCurDataPageSize() - _cachedSz ;
         mvSize = ( UINT32 )mvSize <= len ? mvSize : len ;
         ossMemcpy( _pool + _cachedSz, data, mvSize ) ;
         _cachedSz += mvSize ;
         if ( 0 != len - mvSize )
         {
            _writeData.tuple.columns.offset = offset + mvSize ;
            _writeData.tuple.columns.len = len - mvSize ;
            _writeData.data = data + mvSize ;
         }
         _analysisCache = TRUE ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOBWINDOW_ADDOUTPUTDATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBWINDOW_GETNEXTWRITESEQUENCES, "_rtnLobWindow::getNextWriteSequences" )
   BOOLEAN _rtnLobWindow::getNextWriteSequences( RTN_LOB_TUPLES &tuples )
   {
      PD_TRACE_ENTRY( SDB_RTNLOBWINDOW_GETNEXTWRITESEQUENCES ) ;
      _rtnLobTuple tuple ;
      BOOLEAN hasNext = FALSE ;
      while ( _getNextWriteSequence( tuple ) )
      {
         tuples.push_back( tuple ) ;
         hasNext = TRUE ;
      }

      PD_TRACE_EXIT( SDB_RTNLOBWINDOW_GETNEXTWRITESEQUENCES ) ;
      return hasNext ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBWINDOW__GETNEXTWRITESEQUENCE, "_rtnLobWindow::_getNextWriteSequence" )
   BOOLEAN _rtnLobWindow::_getNextWriteSequence( _rtnLobTuple &tuple )
   {
      PD_TRACE_ENTRY( SDB_RTNLOBWINDOW__GETNEXTWRITESEQUENCE ) ;
      BOOLEAN hasNext = FALSE ;
      MsgLobTuple &t = tuple.tuple ;

      if ( !_analysisCache )
      {
         if ( _getCurDataPageSize() < _writeData.tuple.columns.len )
         {
            t.columns.len = _getCurDataPageSize() ;
            t.columns.sequence = RTN_LOB_GET_SEQUENCE( _curOffset,
                                                       _mergeMeta,
                                                       _logarithmic ) ;
            t.columns.offset = _getCurDataOffset() ;
            tuple.data = _writeData.data ;

            _writeData.tuple.columns.len -= _getCurDataPageSize() ;
            _writeData.tuple.columns.offset += _getCurDataPageSize() ;
            _writeData.data += _getCurDataPageSize() ;
            _curOffset += _getCurDataPageSize() ;
            hasNext = TRUE ;
         }
         else if ( _getCurDataPageSize() == _writeData.tuple.columns.len )
         {
            t.columns.len = _getCurDataPageSize() ;
            t.columns.sequence = RTN_LOB_GET_SEQUENCE( _curOffset,
                                                       _mergeMeta,
                                                       _logarithmic) ;
            t.columns.offset = _getCurDataOffset() ;
            tuple.data = _writeData.data ;

            _curOffset += _getCurDataPageSize() ;
            hasNext = TRUE ;
            _writeData.clear() ;
         }
         else
         {
            /// cache data
            goto done ;
         }
      }
      else if ( _getCurDataPageSize() == (UINT32)_cachedSz )
      {
         t.columns.len = _cachedSz ;
         t.columns.sequence = RTN_LOB_GET_SEQUENCE( _curOffset,
                                                    _mergeMeta,
                                                    _logarithmic) ;
         t.columns.offset = _getCurDataOffset() ;
         tuple.data = _pool ;

         _curOffset += _cachedSz ;
         hasNext = TRUE ;
         _analysisCache = FALSE ;
      }
      else
      {
         SDB_ASSERT( _writeData.empty(), "should be joined before" ) ;
      }

   done:
      PD_TRACE_EXIT( SDB_RTNLOBWINDOW__GETNEXTWRITESEQUENCE ) ;
      return hasNext ;
   }

   void _rtnLobWindow::cacheLastDataOrClearCache()
   {
      if ( _getCurDataPageSize() == (UINT32)_cachedSz )
      {
         _cachedSz = 0 ;
      }

      if ( !_writeData.empty() )
      {
         SDB_ASSERT( _writeData.tuple.columns.len < _getCurDataPageSize(),
                     "Write data len must < _getCurDataPageSize()" ) ;
         SDB_ASSERT( 0 == _cachedSz, "Cached size must be 0" ) ;
         ossMemcpy( _pool + _cachedSz, _writeData.data,
                    _writeData.tuple.columns.len ) ;
         _cachedSz += _writeData.tuple.columns.len ;
         _writeData.clear() ;
      }

      _analysisCache = FALSE ;
   }

   BOOLEAN _rtnLobWindow::getCachedData( _rtnLobTuple &tuple )
   {
      BOOLEAN hasNext = FALSE ;
      MsgLobTuple &t = tuple.tuple ;
      if ( 0 == _cachedSz )
      {
         goto done ;
      }

      t.columns.len = _cachedSz ;
      t.columns.sequence = RTN_LOB_GET_SEQUENCE( _curOffset,
                                                 _mergeMeta,
                                                 _logarithmic ) ;
      t.columns.offset = _getCurDataOffset() ;
      tuple.data = _pool ;

      _curOffset += _cachedSz ;
      hasNext = TRUE ;
      _cachedSz = 0 ;
      _analysisCache = FALSE ;

   done:
      return hasNext ;
   }

   BOOLEAN _rtnLobWindow::getMetaPageData( _rtnLobTuple &tuple )
   {
      BOOLEAN hasNext = FALSE ;
      MsgLobTuple &t = tuple.tuple ;

      if ( 0 == _metaSize )
      {
         goto done ;
      }
      t.columns.len = _metaSize + DMS_LOB_META_LENGTH ;
      t.columns.sequence = DMS_LOB_META_SEQUENCE ;
      t.columns.offset = 0 ;
      tuple.data = _pool + _pageSize ;

      hasNext = TRUE ;
      _metaSize = 0 ;

   done:
      return hasNext ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBWINDOW_PREPARE2READ, "_rtnLobWindow::_rtnLobWindow::prepare2Read" )
   INT32 _rtnLobWindow::prepare2Read( SINT64 lobLen,
                                      SINT64 offset,
                                      UINT32 len,
                                      RTN_LOB_TUPLES &tuples )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBWINDOW_PREPARE2READ ) ;
      SDB_ASSERT( offset < lobLen, "impossible" ) ;
      UINT32 totalRead = 0 ;
      _curOffset = offset ;
      UINT32 maxLen = RTN_MAX_READ_LEN <= ( lobLen - offset ) ?
                      RTN_MAX_READ_LEN : ( lobLen - offset ) ;
      UINT32 needRead = len <= RTN_MIN_READ_LEN ?
                        RTN_MIN_READ_LEN : len ;
      tuples.clear() ;

      while ( _curOffset < lobLen &&
              totalRead < needRead &&
              totalRead < maxLen ) 
      {
         UINT32 offsetOfTuple = RTN_LOB_GET_OFFSET_IN_SEQUENCE( _curOffset,
                                                                _mergeMeta,
                                                                _pageSize ) ;
         UINT32 lenOfTuple = _pageSize - offsetOfTuple ;
         if ( ( lobLen - _curOffset ) < lenOfTuple )
         {
            /// we want to read a whole piece unless hit the end of lob.
            lenOfTuple = lobLen - _curOffset ;
         }

         if ( 0 == lenOfTuple )
         {
            break ;
         }

         UINT32 sequence = RTN_LOB_GET_SEQUENCE( _curOffset,
                                                 _mergeMeta,
                                                 _logarithmic ) ;

         _curOffset += lenOfTuple ;
         totalRead += lenOfTuple ;

         tuples.push_back( _rtnLobTuple( lenOfTuple,
                                         sequence,
                                         offsetOfTuple,
                                         NULL ) ) ;
      }

      PD_TRACE_EXITRC( SDB_RTNLOBWINDOW_PREPARE2READ, rc ) ;
      return rc ;
   }

}

