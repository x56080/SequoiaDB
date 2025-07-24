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

   Source File Name = dmsMetaBlock.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SDB_DMS_META_BLOCK_HPP_
#define SDB_DMS_META_BLOCK_HPP_

#include "dms.hpp"
#include "utilCompression.hpp"

namespace engine
{
   #pragma pack(4)
   /*
      _dmsMetadataBlock defined
   */
   struct _dmsMetadataBlock
   {
      // every records <= 32 bytes go to slot 0
      // every records >32 and <= 64 go to slot 1...
      // every records
      enum deleteListType
      {
         _32 = 0,
         _64,
         _128,
         _256,
         _512,
         _1k,
         _2k,
         _4k,
         _8k,
         _16k,
         _32k,
         _64k,
         _128k,
         _256k,
         _512k,
         _1m,
         _2m,
         _4m,
         _8m,
         _16m,
         _max
      } ;

      CHAR           _collectionName [ DMS_COLLECTION_NAME_SZ+1 ] ;
      UINT16         _flag ;
      UINT16         _blockID ;
      dmsExtentID    _firstExtentID ;
      dmsExtentID    _lastExtentID ;
      UINT32         _numIndexes ;
      dmsRecordID    _deleteList [_max] ;
      dmsExtentID    _indexExtent [DMS_COLLECTION_MAX_INDEX] ;
      UINT32         _logicalID ;
      UINT32         _indexHWCount ;
      UINT32         _attributes ;
      dmsExtentID    _loadFirstExtentID ;
      dmsExtentID    _loadLastExtentID ;
      dmsExtentID    _mbExExtentID ;
      // for stat
      UINT64         _totalRecords ;
      UINT32         _totalDataPages ;
      UINT32         _totalIndexPages ;
      UINT64         _totalDataFreeSpace ;
      UINT64         _totalIndexFreeSpace ;
      UINT32         _totalLobPages ;
      // end

      // This extent is used to store dictionary of the collection. If the
      // dictionary has not been created, the value should be DMS_INVALID_EXTENT.
      dmsExtentID    _dictExtentID ;
      dmsExtentID    _newDictExtentID ;
      SINT32         _dictStatPageID ;
      UINT8          _dictVersion ;
      UINT8          _compressorType ;
      UINT8          _lastCompressRatio ;
      UINT8          _compressFlags ;
      // for stat
      UINT64         _totalLobs ;
      UINT64         _totalOrgDataLen ;
      UINT64         _totalDataLen ;
      // end stat

      // for persistence
      UINT64         _maxGlobTransID ;
      CHAR           _pad2[ 8 ] ;  // reserved
      UINT32         _commitFlag ;
      UINT64         _commitLSN ;
      UINT64         _commitTime ;
      UINT32         _idxCommitFlag ;
      UINT64         _idxCommitLSN ;
      UINT64         _idxCommitTime ;
      UINT32         _lobCommitFlag ;
      UINT64         _lobCommitLSN ;
      UINT64         _lobCommitTime ;
      // end persistence

      // Extend option extent id for collection.
      // If one storage type has its own special options, allocate one seperate
      // page to store them, instead of putting them in this common structure.
      dmsExtentID    _mbOptExtentID ;

      utilCLUniqueID _clUniqueID ;

      CHAR           _pad [ 276 ] ;

      void reset ( const CHAR *clName = NULL,
                   utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL,
                   UINT16 mbID = DMS_INVALID_MBID,
                   UINT32 clLID = DMS_INVALID_CLID,
                   UINT32 attr = 0,
                   UINT8 compressType = UTIL_COMPRESSOR_INVALID )
      {
         INT32 i = 0 ;
         ossMemset( _collectionName, 0, sizeof( _collectionName ) ) ;
         if ( clName )
         {
            ossStrncpy( _collectionName, clName, DMS_COLLECTION_NAME_SZ ) ;
         }
         _clUniqueID = clUniqueID ;
         if ( DMS_INVALID_MBID != mbID )
         {
            DMS_SET_MB_INUSE( _flag ) ;
         }
         else
         {
            DMS_SET_MB_FREE( _flag ) ;
         }
         _blockID = mbID ;
         _firstExtentID = DMS_INVALID_EXTENT ;
         _lastExtentID  = DMS_INVALID_EXTENT ;
         _numIndexes    = 0 ;
         for ( i = 0 ; i < _max ; ++i )
         {
            _deleteList[i].reset() ;
         }
         for ( i = 0 ; i < DMS_COLLECTION_MAX_INDEX ; ++i )
         {
            _indexExtent[i] = DMS_INVALID_EXTENT ;
         }
         _logicalID = clLID ;
         _indexHWCount = 0 ;
         _attributes   = attr ;
         _loadFirstExtentID = DMS_INVALID_EXTENT ;
         _loadLastExtentID  = DMS_INVALID_EXTENT ;
         _mbExExtentID      = DMS_INVALID_EXTENT ;

         _totalRecords           = 0 ;
         _totalDataPages         = 0 ;
         _totalIndexPages        = 0 ;
         _totalDataFreeSpace     = 0 ;
         _totalIndexFreeSpace    = 0 ;
         _totalLobPages          = 0 ;
         _totalLobs              = 0 ;
         _compressorType         = UTIL_COMPRESSOR_INVALID ;
         _dictVersion            = 0 ;
         _dictExtentID           = DMS_INVALID_EXTENT ;
         _newDictExtentID        = DMS_INVALID_EXTENT ;
         _dictStatPageID         = DMS_INVALID_EXTENT ;
         _lastCompressRatio      = 100 ;
         _compressFlags          = UTIL_COMPRESS_ALTERABLE_FLAG ;

         _totalOrgDataLen        = 0 ;
         _totalDataLen           = 0 ;

         _maxGlobTransID         = 0 ;
         _commitFlag             = 0 ;
         _commitLSN              = ~0 ;
         _commitTime             = 0 ;
         _idxCommitFlag          = 0 ;
         _idxCommitLSN           = ~0 ;
         _idxCommitTime          = 0 ;
         _lobCommitFlag          = 0 ;
         _lobCommitLSN           = ~0 ;
         _lobCommitTime          = 0 ;

         _mbOptExtentID          = DMS_INVALID_EXTENT ;

         /// set compressor type
         if ( OSS_BIT_TEST( attr, DMS_MB_ATTR_COMPRESSED ) )
         {
            _compressorType      = compressType ;
         }

         // pad
         ossMemset( _pad2, 0, sizeof( _pad2 ) ) ;
         ossMemset( _pad, 0, sizeof( _pad ) ) ;
      }
   } ;
   typedef _dmsMetadataBlock  dmsMetadataBlock ;
   typedef dmsMetadataBlock   dmsMB ;
   #define DMS_MB_SIZE                 (1024)

   /*
      _dmsMBStatInfo define
   */
   struct _dmsMBStatInfo
   {
      UINT64      _totalRecords ;
      UINT32      _totalDataPages ;
      UINT32      _totalIndexPages ;
      UINT64      _totalDataFreeSpace ;
      UINT64      _totalIndexFreeSpace ;
      UINT32      _totalLobPages ;
      UINT64      _totalLobs ;
      UINT8       _uniqueIdxNum ;
      UINT8       _textIdxNum ;
      UINT8       _globIdxNum ;
      UINT8       _lastCompressRatio ;
      UINT64      _totalOrgDataLen ;
      UINT64      _totalDataLen ;
      UINT32      _startLID ;
      UINT32      _flag ;

      ossAtomic32 _commitFlag ;
      ossAtomic64 _lastLSN ;
      ossAtomic64 _maxGlobTransID ;
      UINT64      _lastWriteTick ;
      BOOLEAN     _isCrash ;

      ossAtomic32 _idxCommitFlag ;
      ossAtomic64 _idxLastLSN ;
      UINT64      _idxLastWriteTick ;
      BOOLEAN     _idxIsCrash ;

      ossAtomic32 _lobCommitFlag ;
      ossAtomic64 _lobLastLSN ;
      UINT64      _lobLastWriteTick ;
      BOOLEAN     _lobIsCrash ;

      // total record count for transaction RC count
      ossAtomic64 _rcTotalRecords ;

      // runtime CRUD statistics monitor
      monCRUDCB _crudCB ;

      // how many operators need to block index creating
      UINT32      _blockIndexCreatingCount ;

      // bitmap to indicate index fields
      ixmIdxHashBitmap _clIdxHashBitmap ;
      ixmIdxHashArray  _idxHashFields[ IXM_IDX_HASH_MAX_INDEX_NUM ] ;

      // global timestamp to support global transaction to
      // fetch MVCC old versions
      // - updated after destination of split
      // - updated after commit of transaction with lock escalated
      ossAtomic64 _globTransAvailTime ;

      void reset()
      {
         _totalRecords           = 0 ;
         _totalDataPages         = 0 ;
         _totalIndexPages        = 0 ;
         _totalDataFreeSpace     = 0 ;
         _totalIndexFreeSpace    = 0 ;
         _totalLobPages          = 0 ;
         _totalLobs              = 0 ;
         _uniqueIdxNum           = 0 ;
         _textIdxNum             = 0 ;
         _globIdxNum             = 0 ;
         _lastCompressRatio      = 100 ;
         _totalOrgDataLen        = 0 ;
         _totalDataLen           = 0 ;
         _startLID               = DMS_INVALID_CLID ;
         _flag                   = 0 ;
         _commitFlag.init( 0 ) ;
         _lastLSN.init( ~0 ) ;
         _lastWriteTick          = 0 ;
         _isCrash                = FALSE ;
         _idxCommitFlag.init( 0 ) ;
         _idxLastLSN.init( ~0 ) ;
         _maxGlobTransID.init( 0 ) ;
         _idxLastWriteTick       = 0 ;
         _idxIsCrash             = FALSE ;
         _lobCommitFlag.init( 0 ) ;
         _lobLastLSN.init( ~0 ) ;
         _lobLastWriteTick       = 0 ;
         _lobIsCrash             = FALSE ;
         _rcTotalRecords.init( 0 ) ;
         _crudCB.reset() ;
         _blockIndexCreatingCount = 0 ;
         _clIdxHashBitmap.resetBitmap() ;
         for ( UINT32 i = 0 ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
         {
            _idxHashFields[ i ].reset() ;
         }
         _globTransAvailTime.init( 0 ) ;
      }

      void updateLastLSN( UINT64 lsn, DMS_FILE_TYPE type )
      {
         if ( OSS_BIT_TEST( type, DMS_FILE_DATA ) )
         {
            _lastLSN.swap( lsn ) ;
         }
         if ( OSS_BIT_TEST( type, DMS_FILE_IDX ) )
         {
            _idxLastLSN.swap( lsn ) ;
         }
         if ( OSS_BIT_TEST( type, DMS_FILE_LOB ) )
         {
            _lobLastLSN.swap( lsn ) ;
         }
      }

      void updateLastLSNWithComp( UINT64 lsn,
                                  DMS_FILE_TYPE type,
                                  BOOLEAN isRollback )
      {
         if ( OSS_BIT_TEST( type, DMS_FILE_DATA ) )
         {
            if ( !_lastLSN.compareAndSwap( DPS_INVALID_LSN_OFFSET, lsn ) )
            {
               if ( !isRollback )
               {
                  _lastLSN.swapGreaterThan( lsn ) ;
               }
               else
               {
                  _lastLSN.swapLesserThan( lsn ) ;
               }
            }
         }
         if ( OSS_BIT_TEST( type, DMS_FILE_IDX ) )
         {
            if ( !_idxLastLSN.compareAndSwap( DPS_INVALID_LSN_OFFSET, lsn ) )
            {
               if ( !isRollback )
               {
                  _idxLastLSN.swapGreaterThan( lsn ) ;
               }
               else
               {
                  _idxLastLSN.swapLesserThan( lsn ) ;
               }
            }
         }
         if ( OSS_BIT_TEST( type, DMS_FILE_LOB ) )
         {
            if ( !_lobLastLSN.compareAndSwap( DPS_INVALID_LSN_OFFSET, lsn ) )
            {
               if ( !isRollback )
               {
                  _lobLastLSN.swapGreaterThan( lsn ) ;
               }
               else
               {
                  _lobLastLSN.swapLesserThan( lsn ) ;
               }
            }
         }
      }

      // compare and update GlobTransID if the one passed in is newer
      // Note: that we only compare serial number with global transaction tag
      //       here. When user use this maxGlobTranID, he may need to consider
      //       max error if needed.
      void updateGlobTransIDWithComp( DPS_TRANS_ID transID )
      {
         _maxGlobTransID.swapGreaterThan( transID.getGlobSN() ) ;
      }

      // get the max GlobTransID 
      UINT64 getMaxGlobTransID( )
      {
         return _maxGlobTransID.peek() ;
      }

      void setIdxHash( INT32 indexID, const CHAR *idxFieldName )
      {
         SDB_ASSERT( indexID >= 0 && indexID < DMS_COLLECTION_MAX_INDEX,
                     "invalid index ID" ) ;
         UINT32 bitIndex = ixmIdxHashBitmap::calcIndex( idxFieldName ) ;
         _clIdxHashBitmap.setBit( bitIndex ) ;
         if ( indexID < IXM_IDX_HASH_MAX_INDEX_NUM )
         {
            _idxHashFields[ indexID ].setField( bitIndex ) ;
         }
      }

      // reset index hash fields from given index
      void resetIdxHashFrom( INT32 indexID )
      {
         SDB_ASSERT( indexID >= 0 && indexID < DMS_COLLECTION_MAX_INDEX,
                     "invalid index ID" ) ;
         _clIdxHashBitmap.resetBitmap() ;
         // reset bitmaps after given index ID
         for ( UINT32 i = indexID ; i < IXM_IDX_HASH_MAX_INDEX_NUM ; ++ i )
         {
            _idxHashFields[ i ].reset() ;
         }
      }

      void resetIdxHashAt( INT32 indexID )
      {
         SDB_ASSERT( indexID >= 0 && indexID < DMS_COLLECTION_MAX_INDEX,
                     "invalid index ID" ) ;
         _idxHashFields[ indexID ].reset() ;
      }

      void mergeIdxHash( INT32 indexID )
      {
         SDB_ASSERT( indexID >= 0 && indexID < DMS_COLLECTION_MAX_INDEX,
                     "invalid index ID" ) ;
         if ( indexID < IXM_IDX_HASH_MAX_INDEX_NUM )
         {
            _idxHashFields[ indexID ].mergeToBitmap( _clIdxHashBitmap ) ;
         }
      }

      BOOLEAN testIdxHash( const ixmIdxHashBitmap &idxHash )
      {
         return _clIdxHashBitmap.hasIntersaction( idxHash ) ;
      }

      BOOLEAN testIdxHash( INT32 indexID, const ixmIdxHashBitmap &idxHash )
      {
         SDB_ASSERT( indexID >= 0 && indexID < DMS_COLLECTION_MAX_INDEX,
                     "invalid index ID" ) ;
         if ( indexID < IXM_IDX_HASH_MAX_INDEX_NUM )
         {
            return _idxHashFields[ indexID ].testBitmap( idxHash ) ;
         }
         return TRUE ;
      }

      BOOLEAN isIdxHashReady() const
      {
         return !( _clIdxHashBitmap.isEmpty() ) ;
      }

      BOOLEAN isIdxHashReady( INT32 indexID ) const
      {
         SDB_ASSERT( indexID >= 0 && indexID < DMS_COLLECTION_MAX_INDEX,
                     "invalid index ID" ) ;
         if ( indexID < IXM_IDX_HASH_MAX_INDEX_NUM )
         {
            return _idxHashFields[ indexID ].isValid() ;
         }
         // for indexes after first 8 ones, always not ready
         return FALSE ;
      }

      _dmsMBStatInfo ()
      : _commitFlag( 0 ),
        _lastLSN( 0 ),
        _maxGlobTransID( 0 ),
        _idxCommitFlag( 0 ),
        _idxLastLSN( 0 ),
        _lobCommitFlag( 0 ),
        _lobLastLSN( 0 ),
        _rcTotalRecords( 0 ),
        _globTransAvailTime( 0 )
      {
         reset() ;
      }

      ~_dmsMBStatInfo ()
      {
         reset() ;
      }
   } ;
   typedef _dmsMBStatInfo dmsMBStatInfo ;

#pragma pack()

} // namespace engine


#endif//SDB_DMS_MB_HPP_