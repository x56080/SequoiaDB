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

   Source File Name = dmsStorageDataCommon.hpp

   Descriptive Name = Common Data Management Service Storage Unit Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/08/2013  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMSSTORAGE_DATACOMMON_HPP_
#define DMSSTORAGE_DATACOMMON_HPP_

#include "dmsStorageBase.hpp"
#include "dmsExtent.hpp"
#include "dpsLogWrapper.hpp"
#include "dpsTransVersionCtrl.hpp"
#include "dpsOp2Record.hpp"
#include "dmsCompress.hpp"
#include "dmsEventHandler.hpp"
#include "dmsExtDataHandler.hpp"

#include "ossMemPool.hpp"
#include "utilInsertResult.hpp"
#include "dmsOprHandler.hpp"
#include "monCB.hpp"
#include "sdbRemoteOperator.hpp"

using namespace bson ;

namespace engine
{
#pragma pack(1)
   /*
      _IDToInsert define
   */
   class _IDToInsert : public SDBObject
   {
   public :
      CHAR _type ;
      CHAR _id[4] ; // _id + '\0'
      OID _oid ;
      _IDToInsert ()
      {
         _type = (CHAR)jstOID ;
         _id[0] = '_' ;
         _id[1] = 'i' ;
         _id[2] = 'd' ;
         _id[3] = 0 ;
         SDB_ASSERT ( sizeof ( _IDToInsert) == 17,
                      "IDToInsert should be 17 bytes" ) ;
      }
   } ;
   typedef class _IDToInsert IDToInsert ;

   /*
      _idToInsert define
   */
   class _idToInsertEle : public BSONElement
   {
   public :
      _idToInsertEle( CHAR* x ) : BSONElement((CHAR*) ( x )){}
   } ;
   typedef class _idToInsertEle idToInsertEle ;

#pragma pack()

#pragma pack(4)

   #define DMS_MB_SIZE                 (1024)

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

      UINT64         _totalLobSize ;
      UINT64         _totalValidLobSize ;

      UINT64         _createTime ;
      UINT64         _updateTime ;

      dmsRecordID    _firstDeletingRID ;
      dmsRecordID    _lastDeletingRID ;
      UINT64         _totalDeletingRecords ;
      UINT64         _totalOverflowRecords ;

      CHAR           _pad [ 212 ] ;

      void reset ( const CHAR *clName = NULL,
                   utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL,
                   UINT16 mbID = DMS_INVALID_MBID,
                   UINT32 clLID = DMS_INVALID_CLID,
                   UINT32 attr = 0,
                   UINT8 compressType = UTIL_COMPRESSOR_INVALID )
      {
         SDB_ASSERT( sizeof( _dmsMetadataBlock ) == DMS_MB_SIZE,
                     "metadata block header should be 1024" ) ;

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
         _totalLobSize           = 0 ;
         _totalValidLobSize      = 0 ;

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

         _createTime             = 0 ;
         _updateTime             = 0 ;

         _firstDeletingRID       = dmsRecordID() ;
         _lastDeletingRID        = dmsRecordID() ;
         _totalDeletingRecords   = 0 ;
         _totalOverflowRecords   = 0 ;

         // pad
         ossMemset( _pad2, 0, sizeof( _pad2 ) ) ;
         ossMemset( _pad, 0, sizeof( _pad ) ) ;
      }
   } ;
   typedef _dmsMetadataBlock  dmsMetadataBlock ;
   typedef dmsMetadataBlock   dmsMB ;

#pragma pack()

   // minimum space slot is 2^5 = 32 byte
   #define DMS_MIN_SPACE_SLOT_SQUQRE_ROOT ( 5 )

   // to avoid small deleted record which can not be reused by average
   // size of records in the same collection, we only split the record if
   // the remain size can at least save the record with average size,
   // or current record ( scale down to 0.8x )
   #define DMS_REMAIN_SIZE_RATIO          ( 0.8 )

   // get space slot to store the record with given size
   OSS_INLINE UINT8 dmsMBGetSpaceSlot( UINT32 recSize )
   {
      UINT8 freeSlot = 0 ;

      // divide by 32 (2^5) first since our first slot is for <32 bytes
      // while loop, divide by 2 every time, find the closest delete slot
      // for example, for a given size 3000, we should go _4k (which is
      // _deleteList[7], using 3000>>5=93
      // then in a loop, first round we have 46, type=1
      // then 23, type=2
      // then 11, type=3
      // then 5, type=4
      // then 2, type=5
      // then 1, type=6
      // finally 0, type=7
      recSize = ( recSize - 1 ) >> DMS_MIN_SPACE_SLOT_SQUQRE_ROOT ;
      while ( recSize != 0 )
      {
         ++ freeSlot ;
         recSize >>= 1 ;
      }

      return freeSlot ;
   }

   /*
      Type to String functions
   */
   void  mbFlag2String ( UINT16 flag, CHAR *pBuffer, INT32 bufSize ) ;
   void  mbAttr2String ( UINT32 attributes, CHAR *pBuffer, INT32 bufSize ) ;

   /*
      _metadataBlockEx define
   */
   struct _metadataBlockEx
   {
      dmsMetaExtent        _header ;
      dmsExtentID          _array[1] ;

      INT32 getFirstExtentID( UINT32 segID, dmsExtentID &extID ) const
      {
         if ( segID >= _header._segNum )
         {
            return SDB_INVALIDARG ;
         }
         UINT32 index = segID << 1 ;
         extID = _array[index] ;
         return SDB_OK ;
      }
      INT32 getLastExtentID( UINT32 segID, dmsExtentID &extID ) const
      {
         if ( segID >= _header._segNum )
         {
            return SDB_INVALIDARG ;
         }
         UINT32 index = ( segID << 1 ) + 1 ;
         extID = _array[index] ;
         return SDB_OK ;
      }
      INT32 setFirstExtentID( UINT32 segID, dmsExtentID extID )
      {
         if ( segID >= _header._segNum )
         {
            return SDB_INVALIDARG ;
         }
         UINT32 index = segID << 1 ;
         _array[index] = extID ;
         return SDB_OK ;
      }
      INT32 setLastExtentID( UINT32 segID, dmsExtentID extID )
      {
         if ( segID >= _header._segNum )
         {
            return SDB_INVALIDARG ;
         }
         UINT32 index = ( segID << 1 ) + 1 ;
         _array[index] = extID ;
         return SDB_OK ;
      }
   } ;
   typedef _metadataBlockEx   dmsMBEx ;

   #define DMS_MME_SZ               (DMS_MME_SLOTS*DMS_MB_SIZE)
   /*
      _dmsMetadataManagementExtent defined
   */
   struct _dmsMetadataManagementExtent : public SDBObject
   {
      dmsMetadataBlock  _mbList [ DMS_MME_SLOTS ] ;

      _dmsMetadataManagementExtent ()
      {
         SDB_ASSERT( DMS_MME_SZ == sizeof( _dmsMetadataManagementExtent ),
                     "MME size error" ) ;
         ossMemset( this, 0, sizeof( _dmsMetadataManagementExtent ) ) ;
      }
   } ;
   typedef _dmsMetadataManagementExtent dmsMetadataManagementExtent ;

   /*
      _dmsMBStatInfo define
   */
   struct _dmsMBStatInfo : public SDBObject
   {
      UINT64      _totalRecords ;
      UINT32      _totalDataPages ;
      UINT32      _totalIndexPages ;
      UINT32      _totalLobPages ;
      UINT64      _totalDataFreeSpace ;
      UINT64      _totalIndexFreeSpace ;
      UINT64      _totalLobs ;
      UINT8       _uniqueIdxNum ;
      UINT8       _textIdxNum ;
      UINT8       _globIdxNum ;
      UINT8       _lastCompressRatio ;
      UINT64      _totalOrgDataLen ;
      UINT64      _totalDataLen ;
      UINT32      _startLID ;
      UINT16      _hasTruncate ;
      UINT16      _flag ;
      UINT64      _totalLobSize ;
      UINT64      _totalValidLobSize ;

      ossAtomic32 _commitFlag ;
      ossAtomic32 _curWriteCount ;
      ossAtomic64 _lastLSN ;
      // FIXME: need "atomic" for DPS_TRANS_ID later
      ossAtomic64 _maxGlobTransID ;
      UINT64      _lastWriteTick ;
      BOOLEAN     _isCrash ;

      ossAtomic32 _idxCommitFlag ;
      ossAtomic64 _idxLastLSN ;
      UINT64      _idxLastWriteTick ;
      BOOLEAN     _idxIsCrash ;

      ossAtomic32 _lobCommitFlag ;
      ossAtomic32 _curLobWriteCount ;
      ossAtomic64 _lobLastLSN ;
      UINT64      _lobLastWriteTick ;
      BOOLEAN     _lobIsCrash ;
      // how many operators need to block index creating
      UINT32      _blockIndexCreatingCount ;

      // total record count for transaction RC count
      ossAtomic64 _rcTotalRecords ;

      // runtime CRUD statistics monitor
      monCRUDCB _crudCB ;

      // bitmap to indicate index fields
      ixmIdxHashBitmap _clIdxHashBitmap ;
      ixmIdxHashArray  _idxHashFields[ IXM_IDX_HASH_MAX_INDEX_NUM ] ;

      // the last search slot of delete list
      UINT8       _lastSearchSlot ;
      // the last search position of delete list
      dmsRecordID _lastSearchRID ;

      // cache of create time
      UINT64      _createTime ;
      // cache of update time
      UINT64      _updateTime ;

      // cache of mb
      CHAR           _collectionName [ DMS_COLLECTION_NAME_SZ+1 ] ;
      UINT16         _blockID ;
      UINT32         _numIndexes ;
      UINT32         _logicalID ;
      UINT32         _attributes ;
      dmsExtentID    _dictExtentID ;
      UINT8          _dictVersion ;
      UINT8          _compressorType ;

      UINT32         _commitFlagSync ;
      UINT64         _commitTime ;
      UINT32         _idxCommitFlagSync ;
      UINT64         _idxCommitTime ;
      UINT32         _lobCommitFlagSync ;
      UINT64         _lobCommitTime ;

      utilCLUniqueID _clUniqueID ;

      UINT64         _totalDeletingRecords ;
      UINT64         _totalOverflowRecords ;

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
         _hasTruncate            = 0 ;
         _flag                   = 0 ;
         _totalLobSize           = 0 ;
         _totalValidLobSize      = 0 ;
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
         _lastSearchSlot = dmsMB::_max ;
         _lastSearchRID.reset() ;
         _createTime             = 0 ;
         _updateTime             = 0 ;

         /// cache info
         ossMemset( _collectionName, 0, sizeof( _collectionName ) ) ;
         _blockID = DMS_INVALID_MBID ;
         _numIndexes = 0 ;
         _logicalID = DMS_INVALID_CLID ;
         _attributes = 0 ;
         _dictExtentID = DMS_INVALID_EXTENT ;
         _dictVersion = 0 ;
         _compressorType = UTIL_COMPRESSOR_INVALID ;

         _commitFlagSync = 0 ;
         _commitTime = 0 ;
         _idxCommitFlagSync = 0 ;
         _idxCommitTime = 0 ;
         _lobCommitFlagSync = 0 ;
         _lobCommitTime = 0 ;

         _clUniqueID = UTIL_UNIQUEID_NULL ;
         _totalDeletingRecords = 0 ;
         _totalOverflowRecords = 0 ;
      }

      void setByMB( const dmsMB *mb )
      {
         _totalRecords = mb->_totalRecords ;
         _rcTotalRecords.init( mb->_totalRecords ) ;
         _totalDataPages = mb->_totalDataPages ;
         _totalIndexPages = mb->_totalIndexPages ;
         _totalDataFreeSpace = mb->_totalDataFreeSpace ;
         _totalIndexFreeSpace = mb->_totalIndexFreeSpace ;
         _totalLobPages = mb->_totalLobPages ;
         _totalLobs = mb->_totalLobs ;
         _lastCompressRatio = mb->_lastCompressRatio ;
         _totalDataLen = mb->_totalDataLen ;
         _totalLobSize = mb->_totalLobSize ;
         _totalValidLobSize = mb->_totalValidLobSize;
         _totalOrgDataLen = mb->_totalOrgDataLen ;
         _startLID = mb->_logicalID ;
         _createTime = mb->_createTime ;
         _updateTime = mb->_updateTime ;
         _maxGlobTransID.init( mb->_maxGlobTransID ) ;

         /// should init lsn
         _lastLSN.init( mb->_commitLSN ) ;
         _idxLastLSN.init( mb->_idxCommitLSN ) ;
         _lobLastLSN.init( mb->_lobCommitLSN ) ;

         /// cache info
         ossStrncpy( _collectionName, mb->_collectionName, DMS_COLLECTION_NAME_SZ ) ;
         _blockID = mb->_blockID ;
         _numIndexes = mb->_numIndexes ;
         _logicalID = mb->_logicalID ;
         _attributes = mb->_attributes ;
         _flag = mb->_flag ;
         _dictExtentID = mb->_dictExtentID ;
         _dictVersion = mb->_dictVersion ;
         _compressorType = mb->_compressorType ;

         updateCommitCache( mb ) ;

         _clUniqueID = mb->_clUniqueID ;
         _totalDeletingRecords = mb->_totalDeletingRecords ;
         _totalOverflowRecords = mb->_totalOverflowRecords ;
      }

      void updateCommitCache( const dmsMB *mb )
      {
         _commitFlagSync = mb->_commitFlag ;
         _commitTime = mb->_commitTime ;
         _idxCommitFlagSync = mb->_idxCommitFlag ;
         _idxCommitTime = mb->_idxCommitTime ;
         _lobCommitFlagSync = mb->_lobCommitFlag ;
         _lobCommitTime = mb->_lobCommitTime ;
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

      // compare and update GlobTransID is the one passed in is newer
      // FIXME: need implement automic compare and swap for DPS_TRANS_ID later
      void updateGlobTransIDWithComp( DPS_TRANS_ID transID )
      {
         _maxGlobTransID.swapGreaterThan( transID ) ;
      }

      // compare and update GTID is the one passed in is newer
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
      void resetIdxHashFrom( INT32 indexID, const dmsMB *mb )
      {
         SDB_ASSERT( indexID >= 0 && indexID < DMS_COLLECTION_MAX_INDEX,
                     "invalid index ID" ) ;
         _numIndexes = mb->_numIndexes ;

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
         if ( indexID < IXM_IDX_HASH_MAX_INDEX_NUM )
         {
            _idxHashFields[ indexID ].reset() ;
         }
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

      UINT32 getAvgDataSize() const
      {
         if ( 0 != _totalRecords )
         {
            // calculate from total data length and total records
            UINT64 avgSize = _totalDataLen / _totalRecords ;
            avgSize = OSS_MAX( DMS_MIN_RECORD_SZ, avgSize ) ;
            avgSize = OSS_MIN( DMS_RECORD_USER_MAX_SZ, avgSize ) ;
            return (UINT32)( avgSize ) ;
         }
         return 0 ;
      }

      void addTotalLobSize( INT64 size )
      {
         ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalLobSize ), size ) ;
      }

      void subTotalLobSize( INT64 size )
      {
         addTotalLobSize( 0 - size ) ;
      }

      void resetTotalLobSize()
      {
         ossAtomicExchange64( OSS_ONCE_UINT64_PTR( _totalLobSize ), 0 ) ;
      }

      void addTotalValidLobSize( INT64 size )
      {
         ossFetchAndAdd64( OSS_ONCE_UINT64_PTR( _totalValidLobSize ), size ) ;
      }

      void resetTotalValidLobSize()
      {
         ossAtomicExchange64( OSS_ONCE_UINT64_PTR( _totalValidLobSize ), 0 ) ;
      }

      _dmsMBStatInfo ()
      : _commitFlag( 0 ),
        _curWriteCount( 0 ),
        _lastLSN( 0 ),
        _maxGlobTransID( 0 ),
        _idxCommitFlag( 0 ),
        _idxLastLSN( 0 ),
        _lobCommitFlag( 0 ),
        _curLobWriteCount( 0 ),
        _lobLastLSN( 0 ),
        _rcTotalRecords( 0 )
      {
         reset() ;
      }

      ~_dmsMBStatInfo ()
      {
         reset() ;
      }
   } ;
   typedef _dmsMBStatInfo dmsMBStatInfo ;

   class _dmsStorageDataCommon ;
   /*
      _dmsMBContext define
   */
   class _dmsMBContext : public _dmsContext
   {
      friend class _dmsStorageDataCommon ;
      private:
         _dmsMBContext() ;
         virtual ~_dmsMBContext() ;
         void _reset () ;

      public:
         virtual string toString () const ;
         virtual INT32  pause () ;
         virtual INT32  resume () ;

         void setSubContext( _IContext *subContext ) ;
         void swap( _dmsMBContext &other ) ;

         OSS_INLINE INT32   mbLock( INT32 lockType ) ;
         OSS_INLINE INT32   mbTryLock( INT32 lockType ) ;
         OSS_INLINE INT32   mbUnlock() ;
         OSS_INLINE BOOLEAN isMBLock( INT32 lockType ) const ;
         OSS_INLINE BOOLEAN isMBLock() const ;
         OSS_INLINE BOOLEAN canResume() const ;

         virtual     UINT16 mbID () const { return _mbID ; }
         OSS_INLINE  dmsMB* mb () { return _mb ; }
         OSS_INLINE  dmsMBStatInfo* mbStat() { return _mbStat ; }
         OSS_INLINE  UINT32 clLID () const { return _clLID ; }
         OSS_INLINE  UINT32 startLID() const { return _startLID ; }
         OSS_INLINE  INT32  mbLockType() const { return _mbLockType ; }

      private:
         OSS_INLINE INT32   _mbLock( INT32 lockType, BOOLEAN isTry ) ;
      private:
         dmsMB             *_mb ;
         dmsMBStatInfo     *_mbStat ;
         monSpinSLatch     *_latch ;
         UINT32            _clLID ;
         UINT32            _startLID ;
         UINT16            _mbID ;
         INT32             _mbLockType ;
         INT32             _resumeType ;
         _IContext         *_pSubContext ;
   };
   typedef _dmsMBContext   dmsMBContext ;

   class _dmsMBContextSubScope : public SDBObject
   {
   public:
      _dmsMBContextSubScope( _dmsMBContext* mbContext, _IContext *subContext ) ;
      ~_dmsMBContextSubScope() ;

   private:
      _dmsMBContext *_mbContext ;
   } ;

   /*
      _dmsMBContext OSS_INLINE functions
   */
   OSS_INLINE INT32 _dmsMBContext::_mbLock( INT32 lockType,
                                            BOOLEAN isTry )
   {
      INT32 rc = SDB_OK ;
      if ( SHARED != lockType && EXCLUSIVE != lockType )
      {
         return SDB_INVALIDARG ;
      }
      if ( _mbLockType == lockType )
      {
         return SDB_OK ;
      }
      // already lock(type not same), need to unlock
      if ( -1 != _mbLockType && SDB_OK != ( rc = pause() ) )
      {
         return rc ;
      }

      // check before lock
      if ( !DMS_IS_MB_INUSE( _mbStat->_flag ) )
      {
         return SDB_DMS_NOTEXIST ;
      }
      if ( _clLID != _mbStat->_logicalID )
      {
         if ( _startLID == _mbStat->_startLID && _mbStat->_hasTruncate )
         {
            return SDB_DMS_TRUNCATED ;
         }
         else
         {
            return SDB_DMS_NOTEXIST ;
         }
      }
      else if ( _startLID != _mbStat->_startLID )
      {
         // start logical IDs are different
         // NOTE: recycle or return cases will keep the logical ID
         if ( (UINT32)DMS_INVALID_CLID == _mbStat->_startLID )
         {
            // collection is recycled by drop or truncate
            if ( _mbStat->_hasTruncate )
            {
               return SDB_DMS_TRUNCATED ;
            }
            else
            {
               return SDB_DMS_NOTEXIST ;
            }
         }
         else if ( (UINT32)DMS_INVALID_CLID == _startLID )
         {
            // recycle collection is returned
            return SDB_RECYCLE_ITEM_NOTEXIST ;
         }
         return SDB_DMS_NOTEXIST ;
      }

      if ( isTry )
      {
         BOOLEAN hasLock = FALSE ;
         hasLock = ( SHARED == lockType ) ?
                   _latch->try_get_shared() : _latch->try_get() ;
         if ( !hasLock )
         {
            return SDB_TIMEOUT ;
         }
      }
      else
      {
         ossLatch( _latch, (OSS_LATCH_MODE)lockType ) ;
      }

      // check after lock
      if ( !DMS_IS_MB_INUSE( _mbStat->_flag ) )
      {
         ossUnlatch( _latch, (OSS_LATCH_MODE)lockType ) ;
         return SDB_DMS_NOTEXIST ;
      }
      if ( _clLID != _mbStat->_logicalID )
      {
         if ( _startLID == _mbStat->_startLID && _mbStat->_hasTruncate )
         {
            ossUnlatch( _latch, (OSS_LATCH_MODE)lockType ) ;
            return SDB_DMS_TRUNCATED ;
         }
         else
         {
            ossUnlatch( _latch, (OSS_LATCH_MODE)lockType ) ;
            return SDB_DMS_NOTEXIST ;
         }
      }
      else if ( _startLID != _mbStat->_startLID )
      {
         // start logical IDs are different
         // NOTE: recycle or return cases will keep the logical ID
         if ( (UINT32)DMS_INVALID_CLID == _mbStat->_startLID )
         {
            // collection is recycled by drop or truncate
            if ( _mbStat->_hasTruncate )
            {
               ossUnlatch( _latch, (OSS_LATCH_MODE)lockType ) ;
               return SDB_DMS_TRUNCATED ;
            }
            else
            {
               ossUnlatch( _latch, (OSS_LATCH_MODE)lockType ) ;
               return SDB_DMS_NOTEXIST ;
            }
         }
         else if ( (UINT32)DMS_INVALID_CLID == _startLID )
         {
            // recycle collection is returned
            ossUnlatch( _latch, (OSS_LATCH_MODE)lockType ) ;
            return SDB_RECYCLE_ITEM_NOTEXIST ;
         }
         ossUnlatch( _latch, (OSS_LATCH_MODE)lockType ) ;
         return SDB_DMS_NOTEXIST ;
      }

      _mbLockType = lockType ;
      _resumeType = -1 ;
      return SDB_OK ;
   }
   OSS_INLINE INT32 _dmsMBContext::mbLock( INT32 lockType )
   {
      return _mbLock( lockType, FALSE ) ;
   }
   OSS_INLINE INT32 _dmsMBContext::mbTryLock( INT32 lockType )
   {
      return _mbLock( lockType, TRUE ) ;
   }
   OSS_INLINE INT32 _dmsMBContext::mbUnlock()
   {
      if ( SHARED == _mbLockType || EXCLUSIVE == _mbLockType )
      {
         ossUnlatch( _latch, (OSS_LATCH_MODE)_mbLockType ) ;
         _resumeType = _mbLockType ;
         _mbLockType = -1 ;
      }
      return SDB_OK ;
   }
   OSS_INLINE BOOLEAN _dmsMBContext::isMBLock( INT32 lockType ) const
   {
      return lockType == _mbLockType ? TRUE : FALSE ;
   }
   OSS_INLINE BOOLEAN _dmsMBContext::isMBLock() const
   {
      if ( SHARED == _mbLockType || EXCLUSIVE == _mbLockType )
      {
         return TRUE ;
      }
      return FALSE ;
   }
   OSS_INLINE BOOLEAN _dmsMBContext::canResume() const
   {
      if ( SHARED == _resumeType || EXCLUSIVE == _resumeType )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   /*
      _dmsRecordRW define
   */
   class _dmsRecordRW
   {
      friend class _dmsStorageDataCommon ;

      public:
         _dmsRecordRW() ;
         _dmsRecordRW( const _dmsRecordRW &recordRW ) ;
         virtual ~_dmsRecordRW() ;

         BOOLEAN           isEmpty() const ;
         _dmsRecordRW      derivePre() const ;
         _dmsRecordRW      deriveNext() const ;
         _dmsRecordRW      deriveOverflow() const ;
         _dmsRecordRW      derive( const dmsRecordID &rid ) const ;

         void              setNothrow( BOOLEAN nothrow ) ;
         BOOLEAN           isNothrow() const ;

         BOOLEAN           isDirectMem() const { return _isDirectMem ; }

         dmsRecordID       getRecordID() const { return _rid ; }

         /*
            When len == 0, Use the record's size
         */
         const dmsRecord*  readPtr( UINT32 len = sizeof( dmsRecord ) ) const ;
         dmsRecord*        writePtr( UINT32 len = sizeof( dmsRecord ) ) ;


         template< typename T >
         const T*          readPtr( UINT32 len = sizeof(T) ) const
         {
            return ( const T* )readPtr( len ) ;
         }

         template< typename T >
         T*                writePtr( UINT32 len = sizeof(T)  )
         {
            return ( T* )writePtr( len ) ;
         }

         std::string       toString() const ;
         void              submit() ;

      private:
         void              _doneAddr() ;

      protected:
         BOOLEAN           _isDirectMem ;
         const dmsRecord   *_ptr ;

      private:
         dmsRecordID       _rid ;
         dmsExtRW          _rw ;
         _dmsStorageDataCommon   *_pData ;
   } ;
   typedef class _dmsRecordRW dmsRecordRW ;

   #define DMS_MME_OFFSET                 ( DMS_SME_OFFSET + DMS_SME_SZ )
   #define DMS_DATASU_EYECATCHER          "SDBDATA"

   // History of data version change:
   // Version  Update in which version    Reason
   //    1             --                 The Initial version.
   //    2             2.0                Support for lzw compression. New
   //                                     dictionary extent information added
   //                                     in MB.
   //    3             2.9                Support for capped collection. A new
   //                                     page for extend option is used, id
   //                                     stored in MB.
   //    16            5.0.3/5.0.4        Support for MVCC.
   // WARNING: next version should start from 17
   #define DMS_COMPRESSION_ENABLE_VER     2
   #define DMS_CAPPED_ENABLE_VER          3
   #define DMS_MVCC_ENABLE_VER            16
   #define DMS_DATASU_CUR_VERSION         DMS_CAPPED_ENABLE_VER
   #define DMS_DATASU_MAX_VERSION         DMS_MVCC_ENABLE_VER

   #define DMS_DATACAPSU_EYECATCHER       "SDBDCAP"
   #define DMS_CONTEXT_MAX_SIZE           (2000)
   #define DMS_RECORDS_PER_EXTENT_SQUARE  4     // value is 2^4=16
   #define DMS_RECORD_OVERFLOW_RATIO      1.2f

   /*
      DMS TRUNCATE TYPE DEFINE
   */
   enum DMS_TRUNC_TYPE
   {
      DMS_TRUNC_LOAD    = 1,
      DMS_TRUNC_ALL     = 2
   } ;

   class _dmsStorageIndex ;
   class _dmsStorageLob ;
   class _dmsStorageUnit ;
   class _pmdEDUCB ;
   class _mthModifier ;

   /*
      _dmsStorageDataCommon defined
   */
   class _dmsStorageDataCommon : public _dmsStorageBase
   {
      friend class _dmsStorageIndex ;
      friend class _dmsStorageUnit ;
      friend class _dmsStorageLob ;
      friend class _dmsRBSSUMgr ;


      struct cmp_str
      {
         bool operator() (const char *a, const char *b)
         {
            return ossStrcmp( a, b ) < 0 ;
         }
      } ;

      typedef ossPoolMap<const CHAR*, UINT16, cmp_str> COLNAME_MAP ;
      typedef ossPoolMap<utilCLUniqueID, UINT16>       COLID_MAP ;
#if defined (_WINDOWS)
      typedef COLNAME_MAP::iterator                         COLNAME_MAP_IT ;
      typedef COLNAME_MAP::const_iterator                   COLNAME_MAP_CIT ;
#else
      typedef ossPoolMap<const CHAR*, UINT16>::iterator       COLNAME_MAP_IT ;
      typedef ossPoolMap<const CHAR*, UINT16>::const_iterator COLNAME_MAP_CIT ;
#endif
      typedef COLID_MAP::iterator    COLID_MAP_IT ;
      typedef COLID_MAP::const_iterator COLID_MAP_CIT ;

      public:
         _dmsStorageDataCommon ( const CHAR *pSuFileName,
                                 dmsStorageInfo *pInfo,
                                 _IDmsEventHolder *pEventHolder ) ;
         virtual ~_dmsStorageDataCommon () ;

         virtual INT32 saveMeta() ;

         virtual void  syncMemToMmap( BOOLEAN *pHasWritten = NULL ) ;

         UINT32 logicalID () const { return _logicalCSID ; }
         dmsStorageUnitID CSID () const { return _CSID ; }

         OSS_INLINE INT32  getMBContext( dmsMBContext **pContext, UINT16 mbID,
                                         UINT32 clLID, UINT32 startLID,
                                         INT32 lockType = -1 );
         OSS_INLINE INT32  getMBContext( dmsMBContext **pContext,
                                         const CHAR* pName,
                                         INT32 lockType = -1 ) ;
         OSS_INLINE INT32  getMBContextByID( dmsMBContext **pContext,
                                             utilCLUniqueID clUniqueID,
                                             INT32 lockType = -1 ) ;

         OSS_INLINE void   releaseMBContext( dmsMBContext *&pContext ) ;

         OSS_INLINE const dmsMBStatInfo* getMBStatInfo( UINT16 mbID ) const ;
         OSS_INLINE const dmsMB* getMBInfo( UINT16 mbID ) const ;

         OSS_INLINE UINT32 getCollectionNum() ;

         INT32 getCollectionMBSlot( ossPoolVector<UINT16> &vecMBSlot ) ;

         OSS_INLINE dmsRecordRW record2RW( const dmsRecordID &record,
                                           UINT16 collectionID ) const ;

         OSS_INLINE INT32 getMBInfo( const CHAR *pName,
                                     UINT16 &mbID,
                                     UINT32 &clLID,
                                     utilCLUniqueID &clUniqueID ) ;

         BOOLEAN isCapped () { return _isCapped; }

         // update extent logical id and expanded meta
         // must hold mb exclusive lock
         INT32         addExtent2Meta( dmsExtentID extID, dmsExtent *extent,
                                       dmsMBContext *context ) ;
         INT32         removeExtentFromMeta( dmsMBContext *context,
                                             dmsExtentID extID,
                                             dmsExtent *extent ) ;

         OSS_INLINE void   updateCreateLobs( UINT32 createLobs ) ;

         void regExtDataHandler( IDmsExtDataHandler *handler )
         {
            _pExtDataHandler = handler ;
         }

         void unregExtDataHandler()
         {
            _pExtDataHandler = NULL ;
         }

         IDmsExtDataHandler *getExtDataHandler()
         {
            return _pExtDataHandler ;
         }

         /// flush mme
         INT32          flushMME( BOOLEAN sync = FALSE, BOOLEAN skipMemSync = FALSE ) ;

         BOOLEAN        isTransSupport( dmsMBContext *context ) const ;

      public:

         // create a new collection for given name, returns collectionID
         INT32 addCollection ( const CHAR *pName,
                               UINT16 *collectionID,
                               utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL,
                               UINT32 attributes = 0,
                               _pmdEDUCB * cb = NULL,
                               SDB_DPSCB *dpscb = NULL,
                               UINT16 initPages = 0,
                               BOOLEAN sysCollection = FALSE,
                               UINT8 compressionType = UTIL_COMPRESSOR_INVALID,
                               UINT32 *logicID = NULL,
                               const BSONObj *extOptions = NULL,
                               const BSONObj *pIdIdxDef = NULL,
                               BOOLEAN addIdxIDIfNotExist = FALSE ) ;

         INT32 dropCollection ( const CHAR *pName,
                                _pmdEDUCB *cb,
                                SDB_DPSCB *dpscb,
                                BOOLEAN sysCollection = TRUE,
                                dmsMBContext *context = NULL,
                                dmsDropCLOptions *options = NULL ) ;

         INT32 truncateCollection ( const CHAR *pName,
                                    _pmdEDUCB *cb,
                                    SDB_DPSCB *dpscb,
                                    BOOLEAN sysCollection = TRUE,
                                    dmsMBContext *context = NULL,
                                    BOOLEAN needChangeCLID = TRUE,
                                    BOOLEAN truncateLob = TRUE,
                                    dmsTruncCLOptions *options = NULL ) ;

         INT32 truncateCollectionLoads( const CHAR *pName,
                                        dmsMBContext *context = NULL ) ;

         INT32 changeCLUniqueID( const MAP_CLNAME_ID& modifyCl,
                                 BOOLEAN changeOtherCL,
                                 utilCSUniqueID csUniqueID,
                                 BOOLEAN isLoadCS,
                                 ossPoolVector<ossPoolString>& clVec ) ;

         INT32 renameCollection ( const CHAR *oldName, const CHAR *newName,
                                  _pmdEDUCB *cb, SDB_DPSCB *dpscb,
                                  BOOLEAN sysCollection = FALSE,
                                  utilCLUniqueID newCLUniqueID = UTIL_UNIQUEID_NULL,
                                  UINT32 *newStartLID = NULL ) ;

         INT32 copyCollection( dmsMBContext *mbContext,
                               const CHAR *newName,
                               utilCLUniqueID newCLUniqueID,
                               pmdEDUCB *cb ) ;
         INT32 recycleCollection( dmsMBContext *mbContext, pmdEDUCB *cb ) ;

         INT32 returnCollection( const CHAR *originName,
                                 const CHAR *recycleName,
                                 dmsReturnOptions &options,
                                 pmdEDUCB *cb,
                                 SDB_DPSCB *dpsCB,
                                 dmsMBContext **returnedMBContext ) ;

         INT32 findCollection ( const CHAR *pName,
                                UINT16 &collectionID,
                                utilCLUniqueID *pClUniqueID = NULL ) ;

         INT32 insertRecord ( dmsMBContext *context,
                              const BSONObj &record,
                              _pmdEDUCB *cb,
                              SDB_DPSCB *dpscb,
                              BOOLEAN mustOID = TRUE,
                              BOOLEAN canUnLock = TRUE,
                              INT64 position = -1,
                              utilInsertResult *insertResult = NULL ) ;

         // if deletedDataPtr = 0, will get from recordID
         // must hold mb exclusive lock
         INT32 deleteRecord ( dmsMBContext *context,
                              const dmsRecordID &recordID,
                              ossValuePtr deletedDataPtr,
                              _pmdEDUCB * cb,
                              SDB_DPSCB *dpscb,
                              IDmsOprHandler *pHandler = NULL,
                              const dmsTransRecordInfo *pInfo = NULL,
                              BOOLEAN isUndo = FALSE ) ;

         // if updatedDataPtr = 0, will get from recordID
         // must hold mb exclusive lock
         INT32 updateRecord ( dmsMBContext *context,
                              const dmsRecordID &recordID,
                              ossValuePtr updatedDataPtr,
                              _pmdEDUCB *cb,
                              SDB_DPSCB *dpscb,
                              _mthModifier &modifier,
                              BSONObj* newRecord = NULL,
                              IDmsOprHandler *pHandler = NULL,
                              utilUpdateResult *pResult = NULL ) ;

         virtual INT32 popRecord( dmsMBContext *context,
                                  INT64 targetID,
                                  _pmdEDUCB *cb,
                                  SDB_DPSCB *dpscb,
                                  INT8 direction = 1,
                                  BOOLEAN byNumber = FALSE ) ;

         // the dataRecord is not owned
         // Caller must hold mb exclusive/shared lock
         INT32 fetch ( dmsMBContext *context,
                       const dmsRecordID &recordID,
                       BSONObj &dataRecord,
                       _pmdEDUCB *cb,
                       BOOLEAN dataOwned = FALSE ) ;

         INT32 loadDictionary( dmsMBContext *context, const CHAR *dictionary,
                               UINT32 dictLen ) ;

         BOOLEAN getDictionary( dmsMBContext *context, const CHAR *&dictionary,
                                UINT32 &dictLen ) ;

         OSS_INLINE _dmsCompressorEntry *getCompressorEntry( UINT16 mbID ) ;

         virtual void incWritePtrCount( INT32 collectionID ) ;

         virtual void decWritePtrCount( INT32 collectionID ) ;

         virtual DMS_FILE_TYPE getFileType() const { return DMS_FILE_DATA ; }

         /*
            Caller must hold the mbContext
         */
         virtual INT32 extractData( const dmsMBContext *mbContext,
                                    const dmsRecordRW &recordRW,
                                    _pmdEDUCB *cb,
                                    dmsRecordData &recordData,
                                    BOOLEAN needIncDataRead = TRUE ) = 0 ;

         virtual void postLoadExt( dmsMBContext *context,
                                   dmsExtent *extAddr,
                                   SINT32 extentID )
         {
         }

         virtual INT32 postDataRestored( dmsMBContext * context )
         {
            return SDB_OK ;
         }

         virtual INT32 dumpExtOptions( dmsMBContext *context,
                                       BSONObj &extOptions ) = 0 ;

         virtual INT32 setExtOptions ( dmsMBContext * context,
                                       const BSONObj & extOptions ) = 0 ;

         BOOLEAN       pushToDeletingList( dmsMBContext *pContext,
                                           dmsRecordRW &recordRW ) ;

         INT32         eraseFromDeletingList( dmsMBContext *pContext,
                                              dmsRecord *pRecord ) ;

      protected:
         virtual INT32 _prepareAddCollection( const BSONObj *extOption,
                                              dmsExtentID &extOptExtent,
                                              UINT16 &extentPageNum ) = 0 ;

         virtual INT32 _onAddCollection( const BSONObj *extOption,
                                         dmsExtentID extOptExtent,
                                         UINT32 extentSize,
                                         UINT16 collectionID ) = 0 ;

         virtual INT32 _onCollectionTruncated( dmsMBContext *context )
         {
            return SDB_OK ;
         }

         virtual void _onAllocExtent( dmsMBContext *context,
                                      dmsExtent *extAddr,
                                      SINT32 extentID ) = 0 ;

         virtual INT32 _prepareInsertData( const BSONObj &record,
                                           BOOLEAN mustOID,
                                           pmdEDUCB *cb,
                                           dmsRecordData &recordData,
                                           BOOLEAN &memReallocate,
                                           INT64 position ) = 0 ;

         virtual INT32 _getRecordPosition( const dmsRecordID &rid,
                                           const dmsRecordData &recordData,
                                           INT64 &position ) = 0 ;

         virtual INT32 _checkMarkInsert( dmsMBContext *context,
                                         const DPS_TRANS_ID &transID,
                                         const BSONObj &insertObj,
                                         pmdEDUCB *cb,
                                         INT64 &position,
                                         BOOLEAN &markInsert,
                                         dmsRecordID &foundRID,
                                         dmsRecordData &recordData,
                                         dmsRecordRW &recordRW ) = 0 ;

         virtual INT32 _doMarkInsert( dmsMBContext *context,
                                      pmdEDUCB *cb,
                                      dmsExtRW &extRW,
                                      dmsRecordID &foundRID,
                                      UINT32 dmsRecordSize,
                                      dmsRecordData &recordData ) = 0 ;

         virtual INT32 _allocRecordSpace( dmsMBContext *context,
                                          UINT32 size,
                                          dmsRecordID &foundRID,
                                          _pmdEDUCB *cb ) = 0 ;

         virtual INT32 _allocRecordSpaceByPos( dmsMBContext *context,
                                               UINT32 size,
                                               INT64 position,
                                               dmsRecordID &foundRID,
                                               _pmdEDUCB *cb ) = 0 ;

         virtual INT32 _extentInsertRecord( dmsMBContext *context,
                                            dmsExtRW &extRW,
                                            dmsRecordRW &recordRW,
                                            const dmsRecordData &recordData,
                                            UINT32 recordSize,
                                            _pmdEDUCB *cb,
                                            BOOLEAN isInsert = TRUE ) = 0 ;

         virtual void _postInsertRecord( dmsMBContext *context,
                                         dmsExtRW &extRW,
                                         dmsRecordRW &recordRW,
                                         const dmsRecordData &recordData,
                                         UINT32 recordSize,
                                         _pmdEDUCB *cb ) = 0 ;

         virtual INT32 _operationPermChk( DMS_ACCESS_TYPE accessType ) = 0 ;

         virtual INT32 _extentUpdatedRecord( dmsMBContext *context,
                                             dmsExtRW &extRW,
                                             dmsRecordRW &recordRW,
                                             const dmsRecordData &recordData,
                                             const BSONObj &newObj,
                                             _pmdEDUCB *cb,
                                             IDmsOprHandler *pHandler,
                                             utilUpdateResult *pResult,
                                             dpsUnqIdxHashArray *pNewUnqIdxHashArray,
                                             dpsUnqIdxHashArray *pOldUnqIdxHashArray,
                                             const ixmIdxHashBitmap &idxHashBitmap ) = 0 ;

         virtual INT32 _extentRemoveRecord( dmsMBContext *context,
                                            dmsExtRW &extRW,
                                            dmsRecordRW &recordRW,
                                            _pmdEDUCB *cb,
                                            BOOLEAN decCount = TRUE ) = 0 ;

         // Calculate the final size needed by the record. Records of different
         // type may have different strategy, such as reservation for update,
         // space for meta data, etc.
         virtual void _finalRecordSize( UINT32 &size,
                                        const dmsRecordData &recordData,
                                        BOOLEAN markInsert ) = 0 ;

         virtual INT32 _onInsertFail( dmsMBContext *context,
                                      BOOLEAN hasInsert,
                                      dmsRecordID rid,
                                      SDB_DPSCB *dpscb,
                                      ossValuePtr dataPtr,
                                      _pmdEDUCB *cb,
                                      const dmsTransRecordInfo *pInfo ) = 0 ;

         virtual INT32  _onOpened() ;
         virtual void   _onClosed() ;
         virtual INT32  _onMapMeta( UINT64 curOffSet, BOOLEAN isCreateNew ) ;
         virtual void   _onRestore() ;
         virtual INT32  _onFlushDirty( BOOLEAN force, BOOLEAN sync ) ;

         virtual void   _onHeaderUpdated( UINT64 updateTime = 0 )
         {
            _dmsStorageBase::_onHeaderUpdated( updateTime ) ;
            if ( NULL != _dmsHeader && NULL != _pStorageInfo )
            {
               _pStorageInfo->_updateTime = _dmsHeader->_updateTime ;
            }
         }

         INT32 _copyIndexesWithoutTypes( dmsMBContext *oldContext,
                                         dmsMBContext *newContext,
                                         _pmdEDUCB *cb,
                                         UINT16 types ) ;
         INT32 _dropIndexesWithTypes( dmsMBContext *context,
                                      _pmdEDUCB *cb,
                                      UINT16 types,
                                      ossPoolVector< bson::BSONObj > *droppedIndexList = NULL ) ;

      private:
         virtual UINT64 _dataOffset() ;
         virtual UINT32 _curVersion() const ;
         virtual UINT32 _maxSupportedVersion() const ;
         virtual INT32  _checkVersion( dmsStorageUnitHeader *pHeader ) ;
         virtual INT32  _onCreate( OSSFILE *file, UINT64 curOffSet ) ;

         virtual INT32  _onMarkHeaderValid( UINT64 &lastLSN,
                                            BOOLEAN sync,
                                            UINT64 lastTime,
                                            BOOLEAN &setHeadCommFlgValid ) ;

         virtual INT32  _onMarkHeaderInvalid( INT32 collectionID ) ;

         virtual UINT64 _getOldestWriteTick() const ;

      protected:
         OSS_INLINE const CHAR* _clFullName ( const CHAR *clName,
                                              CHAR *clFullName,
                                              UINT32 fullNameLen ) ;
         OSS_INLINE void _overflowSize( UINT32 &size ) ;

         void _attach ( _dmsStorageIndex *pIndexSu ) ;
         void _detach () ;

         void _attachLob( _dmsStorageLob *pLobSu ) ;
         void _detachLob() ;

         void _setCompressor( dmsMBContext *context ) ;
         void _rmCompressor( _dmsMBContext *context ) ;

         // This function allocates a new extent. When the extent is allocated,
         // different storage types( sub classes of this base class ) may have
         // different further in-extent initialize operations. The parameter
         // 'deepInit' specifies if those operations should happen.
         INT32 _allocateExtent ( dmsMBContext *context,
                                 UINT16 numPages,
                                 BOOLEAN deepInit = TRUE,
                                 BOOLEAN add2LoadList = FALSE,
                                 dmsExtentID *allocExtID = NULL ) ;

         INT32 _logDPS( SDB_DPSCB *dpsCB, dpsMergeInfo &info,
                        _pmdEDUCB *cb, dmsMBContext *context,
                        dmsExtentID extLID, BOOLEAN needUnLock,
                        DMS_FILE_TYPE type, UINT32 *clLID = NULL ) ;

         INT32 _freeExtent ( dmsExtentID extentID, INT32 collectionID ) ;
         INT32 _freeExtent ( dmsMBContext *context, dmsExtentID extentID ) ;

         void _increaseMBStat ( utilCLUniqueID clUniqueID,
                                dmsMBStatInfo * mbStat,
                                _pmdEDUCB * cb ) ;
         void _decreaseMBStat ( utilCLUniqueID clUniqueID,
                                dmsMBStatInfo * mbStat,
                                _pmdEDUCB * cb ) ;

         void _onMBUpdated( UINT16 mbID ) ;

         BOOLEAN _isMBSlotInUse( UINT16 mbID ) const
         {
            return _slotBitmap.testBit( mbID ) ;
         }

         UINT16  _nextUsedMBSlot( UINT16 pos ) const
         {
            INT32 slot = _slotBitmap.nextSetBitPos( pos ) ;
            if ( slot < 0 || slot >= DMS_MME_SLOTS )
            {
               return DMS_INVALID_MBID ;
            }
            return (UINT16)slot ;
         }

         DMS_FILE_TYPE _getAllFileType() const ;

         virtual INT32 _ensureNewCollection( UINT16 mbID ) ;

      private:
         void               _initializeMME () ;

         OSS_INLINE INT32 _collectionInsert( const CHAR *pName,
                                             UINT16 mbID,
                                             utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL ) ;
         OSS_INLINE INT32 _collectionChangeID( utilCLUniqueID orignal,
                                               utilCLUniqueID dest,
                                               UINT16 mbID ) ;
         OSS_INLINE INT32 _collectionChangeName( const CHAR *pOld,
                                                 const CHAR *pNew,
                                                 utilCLUniqueID oldID = UTIL_UNIQUEID_NULL,
                                                 utilCLUniqueID newID = UTIL_UNIQUEID_NULL ) ;

         OSS_INLINE UINT16 _collectionNameLookup( const CHAR *pName ) ;
         OSS_INLINE UINT16 _collectionIdLookup( utilCLUniqueID clUniqueID ) ;

         OSS_INLINE void _collectionRemove( const CHAR *pName,
                                            utilCLUniqueID clUniqueID = UTIL_UNIQUEID_NULL ) ;
         OSS_INLINE void _collectionMapCleanup () ;

         INT32          _logDPS( SDB_DPSCB *dpsCB, dpsMergeInfo &info,
                                 _pmdEDUCB * cb, ossSLatch *pLatch,
                                 OSS_LATCH_MODE mode, BOOLEAN &locked,
                                 UINT32 clLID, dmsExtentID extLID ) ;

         INT32          _initCompressorEntry( UINT16 mbID ) ;

         INT32          _setCompressorEntry ( UINT16 mbID ) ;

         INT32          _truncateCollection ( dmsMBContext *context,
                                              BOOLEAN needChangeCLID = TRUE ) ;

         INT32          _truncateCollectionLoads( dmsMBContext *context ) ;

         /*
            Caller must hold the mbContext
         */
         UINT32         _getRecordDataLen( const dmsRecord *pRecord ) ;

         OSS_INLINE UINT32  _getFactor () const ;

         INT32          _insertIndexes( dmsMBContext *context,
                                        dmsExtentID extLID,
                                        BSONObj &inputObj,
                                        const dmsRecordID &rid,
                                        pmdEDUCB * cb,
                                        IDmsOprHandler *pOprHandle,
                                        utilWriteResult *insertResult,
                                        dpsUnqIdxHashArray *pUnqIdxHashArray ) ;

      //private:
      protected:
         dmsMetadataManagementExtent         *_dmsMME ;     // 4MB

         // latch for each MB. For normal record SIUD, shared latches are
         // requested exclusive latch on mblock is only when changing
         // metadata (say add an extent into the MB, or create/drop the MB)
         typedef _utilSparseArray<monSpinSLatch, DMS_MME_SLOTS>   MBLOCK_ARRAY ;
         MBLOCK_ARRAY                        _mblock ;

         //monSpinSLatch                       _mblock [ DMS_MME_SLOTS ] ;
         //dmsMBStatInfo                       _mbStatInfo [ DMS_MME_SLOTS ] ;

         typedef _utilSparseArray<dmsMBStatInfo, DMS_MME_SLOTS>   MBSTAT_ARRAY ;
         MBSTAT_ARRAY                        _mbStatInfo ;

         monSpinSLatch                       _metadataLatch ;
         COLNAME_MAP                         _collectionNameMap ;
         COLID_MAP                           _collectionIDMap ;
         UINT32                              _logicalCSID ;
         dmsStorageUnitID                    _CSID ;

         typedef _utilStackBitmap<DMS_MME_SLOTS>                  MBSLOT_BITMAP ;
         MBSLOT_BITMAP                       _slotBitmap ;

         UINT32                              _mmeSegID ;
         BOOLEAN                             _isCapped ;

         vector<dmsMBContext*>               _vecContext ;
         monSpinXLatch                       _latchContext ;

         _dmsStorageIndex                    *_pIdxSU ;
         _dmsStorageLob                      *_pLobSU ;

         typedef _utilSparseArray<_dmsCompressorEntry, DMS_MME_SLOTS>   COMPRESSENTRY_ARRAY ;
         COMPRESSENTRY_ARRAY                 _compressorEntry ;

         //_dmsCompressorEntry                 _compressorEntry[ DMS_MME_SLOTS ] ;

         _IDmsEventHolder                    *_pEventHolder ;
         _IDmsExtDataHandler                 *_pExtDataHandler ;

   };
   typedef _dmsStorageDataCommon dmsStorageDataCommon ;

   /*
      OSS_INLINE functions :
   */
   OSS_INLINE INT32 _dmsStorageDataCommon::_collectionInsert( const CHAR * pName,
                                                              UINT16 mbID,
                                                              utilCLUniqueID clUniqueID )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN isInserted = FALSE ;
      CHAR *clNamePtr = ossStrdup( pName ) ;

      if ( NULL == clNamePtr )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate dup string failed." ) ;
         goto error ;
      }

      try
      {
         /// insert name
         if ( ! _collectionNameMap.insert( COLNAME_MAP::value_type( clNamePtr, mbID ) ).second )
         {
            /// already exist
            rc = SDB_DMS_EXIST ;
            goto error ;
         }

         isInserted = TRUE ;

         /// insert id
         if ( UTIL_IS_VALID_CLUNIQUEID( clUniqueID ) )
         {
            if ( ! _collectionIDMap.insert( COLID_MAP::value_type( clUniqueID, mbID ) ).second )
            {
               rc = SDB_DMS_EXIST ;
               goto error ;
            }
         }

         /// set bitmap
         _slotBitmap.setBit( mbID ) ;
         dmsGetGlobalObjectStat()->_clNum.inc() ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Insert mbid into maps failed, exception: %s, rc: %d",
                 e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( isInserted )
      {
         _collectionNameMap.erase( clNamePtr ) ;
      }
      SAFE_OSS_FREE( clNamePtr ) ;
      goto done ;
   }

   OSS_INLINE INT32 _dmsStorageDataCommon::_collectionChangeName( const CHAR *pOld,
                                                                  const CHAR *pNew,
                                                                  utilCLUniqueID oldID,
                                                                  utilCLUniqueID newID )
   {
      INT32 rc = SDB_OK ;
      UINT16 mbID = DMS_INVALID_MBID ;
      const CHAR *pOldItemName = NULL ;
      BOOLEAN changeID = FALSE ;
      BOOLEAN hasInsertName = FALSE ;
      COLNAME_MAP::iterator itOldMap ;
      CHAR *clNamePtr = ossStrdup( pNew ) ;

      if ( UTIL_UNIQUEID_NULL != newID && oldID != newID )
      {
         changeID = TRUE ;
      }

      if ( NULL == clNamePtr )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate dup string failed." ) ;
         goto error ;
      }

      /// find the old
      itOldMap = _collectionNameMap.find( pOld ) ;
      if ( itOldMap == _collectionNameMap.end() )
      {
         SDB_ASSERT( FALSE, "Collection is not exist" ) ;
         /// not found
         rc = SDB_DMS_NOTEXIST ;
         goto error ;
      }

      mbID = itOldMap->second ;
      pOldItemName = itOldMap->first ;

      /// insert new
      try
      {
         if ( ! _collectionNameMap.insert( COLNAME_MAP::value_type( clNamePtr, mbID ) ).second )
         {
            rc = SDB_DMS_EXIST ;
            goto error ;
         }

         hasInsertName = TRUE ;

         if ( changeID && UTIL_IS_VALID_CLUNIQUEID( newID ) )
         {
            if ( ! _collectionIDMap.insert( COLID_MAP::value_type( newID, mbID ) ).second )
            {
               rc = SDB_DMS_EXIST ;
               goto error ;
            }
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception when insert collection map: %s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

      /// remove old
      _collectionNameMap.erase( itOldMap ) ;
      SDB_OSS_FREE( const_cast<CHAR *>( pOldItemName ) ) ;

      if ( changeID )
      {
         _collectionIDMap.erase( oldID ) ;
      }

   done:
      return rc ;
   error:
      if ( hasInsertName )
      {
         _collectionNameMap.erase( clNamePtr ) ;
      }
      SAFE_OSS_FREE( clNamePtr ) ;
      goto done ;
   }

   OSS_INLINE INT32 _dmsStorageDataCommon::_collectionChangeID( utilCLUniqueID orignal,
                                                                utilCLUniqueID dest,
                                                                UINT16 mbID )
   {
      INT32 rc = SDB_OK ;
      COLID_MAP::iterator itIDMap ;

      /// first find old
      itIDMap = _collectionIDMap.find( orignal ) ;
      if ( itIDMap != _collectionIDMap.end() )
      {
         UINT16 tmpID = itIDMap->second ;

         if ( DMS_INVALID_MBID == mbID )
         {
            mbID = tmpID ;
         }
         else if ( mbID != tmpID )
         {
            SDB_ASSERT( tmpID == mbID, "MBID must be the same" ) ;
            PD_LOG( PDERROR, "MBID(%u) is not the same with maps(%u)", mbID, tmpID ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      if ( DMS_INVALID_MBID == mbID )
      {
         PD_LOG( PDERROR, "Invalid MBID(%u)", mbID ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( UTIL_IS_VALID_CLUNIQUEID( dest ) )
      {
         /// insert new
         try
         {
            if ( ! _collectionIDMap.insert( COLID_MAP::value_type( dest, mbID ) ).second )
            {
               rc = SDB_DMS_EXIST ;
               goto error ;
            }
         }
         catch( std::exception &e )
         {
            PD_LOG( PDERROR, "Occur exception when insert collection IDMap: %s", e.what() ) ;
            rc = ossException2RC( &e ) ;
            goto error ;
         }
      }

      /// erase the old
      if ( itIDMap != _collectionIDMap.end() )
      {
         _collectionIDMap.erase( itIDMap ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   OSS_INLINE UINT16 _dmsStorageDataCommon::_collectionNameLookup( const CHAR * pName )
   {
      UINT16 mbID = DMS_INVALID_MBID ;
      if ( pName )
      {
         COLNAME_MAP_CIT it = _collectionNameMap.find( pName ) ;
         if ( it != _collectionNameMap.end() )
         {
            mbID = it->second ;
         }
      }
      return mbID ;
   }
   OSS_INLINE UINT16 _dmsStorageDataCommon::_collectionIdLookup( utilCLUniqueID clUniqueID )
   {
      UINT16 mbID = DMS_INVALID_MBID ;
      if ( UTIL_IS_VALID_CLUNIQUEID( clUniqueID ) )
      {
         COLID_MAP_CIT it = _collectionIDMap.find( clUniqueID ) ;
         if ( it != _collectionIDMap.end() )
         {
            mbID = it->second ;
         }
      }
      return mbID ;
   }
   OSS_INLINE void _dmsStorageDataCommon::_collectionRemove( const CHAR * pName,
                                                             utilCLUniqueID clUniqueID )
   {
      COLNAME_MAP_IT it = _collectionNameMap.find( pName ) ;
      if ( _collectionNameMap.end() != it )
      {
         /// unset bit
         _slotBitmap.clearBit( it->second ) ;
         dmsGetGlobalObjectStat()->_clNum.dec() ;
         const CHAR *tp = (*it).first ;
         _collectionNameMap.erase( it ) ;
         SDB_OSS_FREE( const_cast<CHAR *>(tp) ) ;
      }

      if ( UTIL_IS_VALID_CLUNIQUEID( clUniqueID ) )
      {
         _collectionIDMap.erase( clUniqueID ) ;
      }
   }
   OSS_INLINE void _dmsStorageDataCommon::_collectionMapCleanup ()
   {
      COLNAME_MAP_CIT it = _collectionNameMap.begin() ;

      for ( ; it != _collectionNameMap.end() ; ++it )
      {
         SDB_OSS_FREE( const_cast<CHAR *>(it->first) ) ;
      }
      dmsGetGlobalObjectStat()->_clNum.sub( _collectionNameMap.size() ) ;
      _collectionNameMap.clear() ;
      _collectionIDMap.clear() ;
      _slotBitmap.resetBitmap() ;
   }
   OSS_INLINE UINT32 _dmsStorageDataCommon::getCollectionNum()
   {
      ossScopedLock lock( &_metadataLatch, SHARED ) ;
      return (UINT32)_collectionNameMap.size() ;
   }
   OSS_INLINE dmsRecordRW _dmsStorageDataCommon::record2RW( const dmsRecordID &record,
                                                            UINT16 collectionID ) const
   {
      dmsRecordRW rRW ;
      rRW._pData = const_cast<_dmsStorageDataCommon*>( this ) ;
      rRW._rid = record ;
      rRW._rw = extent2RW( record._extent, collectionID ) ;
      rRW._doneAddr() ;
      return rRW ;
   }

   OSS_INLINE const CHAR* _dmsStorageDataCommon::_clFullName( const CHAR *clName,
                                                              CHAR * clFullName,
                                                              UINT32 fullNameLen )
   {
      SDB_ASSERT( fullNameLen > DMS_COLLECTION_FULL_NAME_SZ,
                  "Collection full name len error" ) ;
      ossStrncat( clFullName, getSuName(), DMS_COLLECTION_SPACE_NAME_SZ ) ;
      ossStrncat( clFullName, ".", 1 ) ;
      ossStrncat( clFullName, clName, DMS_COLLECTION_NAME_SZ ) ;
      clFullName[ DMS_COLLECTION_FULL_NAME_SZ ] = 0 ;

      return clFullName ;
   }
   OSS_INLINE void _dmsStorageDataCommon::_overflowSize( UINT32 &size )
   {
      if ( _pStorageInfo && _pStorageInfo->_overflowRatio > 0 )
      {
         size += ( size * _pStorageInfo->_overflowRatio + 50 ) / 100 ;
      }
      else if ( !_pStorageInfo )
      {
         size = size * DMS_RECORD_OVERFLOW_RATIO ;
      }
   }
   OSS_INLINE void _dmsStorageDataCommon::updateCreateLobs( UINT32 createLobs )
   {
      if ( _dmsHeader && _dmsHeader->_createLobs != createLobs )
      {
         _dmsHeader->_createLobs = createLobs ;
         _onHeaderUpdated() ;

         /// flush to file
         flushHeader( isSyncDeep() ) ;
      }
   }
   OSS_INLINE INT32 _dmsStorageDataCommon::getMBContext( dmsMBContext ** pContext,
                                                         UINT16 mbID,
                                                         UINT32 clLID,
                                                         UINT32 startLID,
                                                         INT32 lockType )
   {
      if ( mbID >= DMS_MME_SLOTS )
      {
         return SDB_INVALIDARG ;
      }

      dmsMBStatInfo *pMBStat = NULL ;
      dmsMBContext *pTmpContext = NULL ;

      // metadata shared lock
      if ( (UINT32)DMS_INVALID_CLID == clLID ||
           (UINT32)DMS_INVALID_CLID == startLID )
      {
         _metadataLatch.get_shared() ;
         pMBStat = &( _mbStatInfo[mbID] ) ;

         if ( (UINT32)DMS_INVALID_CLID == clLID )
         {
            clLID = pMBStat->_logicalID ;
         }
         if ( (UINT32)DMS_INVALID_CLID == startLID )
         {
            startLID = pMBStat->_startLID ;
         }
         _metadataLatch.release_shared() ;
      }

      // context lock
      _latchContext.get() ;
      if ( _vecContext.size () > 0 )
      {
         pTmpContext = _vecContext.back () ;
         _vecContext.pop_back () ;
      }
      else
      {
         pTmpContext = SDB_OSS_NEW dmsMBContext ;
      }
      _latchContext.release() ;

      if ( !pTmpContext )
      {
         return SDB_OOM ;
      }

      pTmpContext->_clLID = clLID ;
      pTmpContext->_startLID = startLID ;
      pTmpContext->_mbID = mbID ;
      pTmpContext->_mb = &_dmsMME->_mbList[mbID] ;
      pTmpContext->_mbStat = &(_mbStatInfo[mbID]) ;
      pTmpContext->_latch = &_mblock[mbID] ;

      if ( SHARED == lockType || EXCLUSIVE == lockType )
      {
         INT32 rc = pTmpContext->mbLock( lockType ) ;
         if ( rc )
         {
            releaseMBContext( pTmpContext ) ;
            return rc ;
         }
      }

      *pContext = pTmpContext ;

      return SDB_OK ;
   }

   OSS_INLINE INT32 _dmsStorageDataCommon::getMBContext( dmsMBContext **pContext,
                                                         const CHAR* pName,
                                                         INT32 lockType )
   {
      UINT16 mbID = DMS_INVALID_MBID ;
      UINT32 clLID = DMS_INVALID_CLID ;
      UINT32 startLID = DMS_INVALID_CLID ;

      // metadata shared lock
      _metadataLatch.get_shared() ;
      mbID = _collectionNameLookup( pName ) ;
      if ( DMS_INVALID_MBID != mbID )
      {
         dmsMBStatInfo *pMBStat = &( _mbStatInfo[mbID] ) ;
         clLID = pMBStat->_logicalID ;
         startLID = pMBStat->_startLID ;
      }
      _metadataLatch.release_shared() ;

      if ( DMS_INVALID_MBID == mbID )
      {
         return SDB_DMS_NOTEXIST ;
      }
      return getMBContext( pContext, mbID, clLID, startLID, lockType ) ;
   }

   OSS_INLINE INT32 _dmsStorageDataCommon::getMBContextByID( dmsMBContext **pContext,
                                                             utilCLUniqueID clUniqueID,
                                                             INT32 lockType )
   {
      UINT16 mbID = DMS_INVALID_MBID ;
      UINT32 clLID = DMS_INVALID_CLID ;
      UINT32 startLID = DMS_INVALID_CLID ;

      // metadata shared lock
      _metadataLatch.get_shared() ;
      mbID = _collectionIdLookup( clUniqueID ) ;
      if ( DMS_INVALID_MBID != mbID )
      {
         dmsMBStatInfo *pMBStat = &( _mbStatInfo[mbID] ) ;
         clLID = pMBStat->_logicalID ;
         startLID = pMBStat->_startLID ;
      }
      _metadataLatch.release_shared() ;

      if ( DMS_INVALID_MBID == mbID )
      {
         return SDB_DMS_NOTEXIST ;
      }
      return getMBContext( pContext, mbID, clLID, startLID, lockType ) ;
   }

   OSS_INLINE void _dmsStorageDataCommon::releaseMBContext( dmsMBContext *&pContext )
   {
      if ( !pContext )
      {
         return ;
      }
      pContext->mbUnlock() ;

      _latchContext.get() ;
      if ( _vecContext.size() < DMS_CONTEXT_MAX_SIZE )
      {
         pContext->_reset() ;
         try
         {
            _vecContext.push_back( pContext ) ;
         }
         catch( ... )
         {
            SDB_OSS_DEL pContext ;
         }
      }
      else
      {
         SDB_OSS_DEL pContext ;
      }
      _latchContext.release() ;
      pContext = NULL ;
   }

   OSS_INLINE _dmsCompressorEntry *_dmsStorageDataCommon::getCompressorEntry( UINT16 mbID )
   {
      SDB_ASSERT( DMS_INVALID_MBID != mbID, "mb ID is invalid" ) ;
      return &_compressorEntry[ mbID ] ;
   }

   OSS_INLINE const dmsMBStatInfo* _dmsStorageDataCommon::getMBStatInfo( UINT16 mbID ) const
   {
      if ( mbID >= DMS_MME_SLOTS )
      {
         return NULL ;
      }
      return &_mbStatInfo[ mbID ] ;
   }

   OSS_INLINE const dmsMB* _dmsStorageDataCommon::getMBInfo( UINT16 mbID ) const
   {
      if ( !_dmsMME || mbID >= DMS_MME_SLOTS )
      {
         return NULL ;
      }
      return &(_dmsMME->_mbList[ mbID ]) ;
   }

   OSS_INLINE UINT32 _dmsStorageDataCommon::_getFactor() const
   {
      return 16 + 14 - pageSizeSquareRoot() ;
   }

   OSS_INLINE INT32 _dmsStorageDataCommon::getMBInfo( const CHAR *pName,
                                                      UINT16 &mbID,
                                                      UINT32 &clLID,
                                                      utilCLUniqueID &clUniqueID )
   {
      INT32 rc = SDB_OK ;

      clUniqueID = UTIL_UNIQUEID_NULL ;

      if ( NULL == pName )
      {
         rc = SDB_INVALIDARG ;
      }
      else
      {
         // metadata shared lock
         ossScopedLock lock( &_metadataLatch, SHARED ) ;
         mbID = _collectionNameLookup( pName ) ;
         if ( DMS_INVALID_MBID != mbID )
         {
            const dmsMBStatInfo *pMBStat = &( _mbStatInfo[ mbID ] ) ;
            clLID = pMBStat->_logicalID ;
            clUniqueID = pMBStat->_clUniqueID ;
         }
         else
         {
            rc = SDB_DMS_NOTEXIST ;
         }
      }

      return rc ;
   }

   /*
      Tool Functions
   */
   BOOLEAN  dmsIsRecordIDValid( const BSONElement &oidEle,
                                BOOLEAN allowEOO,
                                const CHAR **pErrStr = NULL ) ;

   /// crud statistics function
#define DMS_MBSTAT_ONCE_INC( _monAppCB_, _mbContext_, op, delta )             \
   {                                                                          \
      if ( NULL != _monAppCB_ && NULL != _mbContext_ )                        \
      {                                                                       \
         _mbContext_->mbStat()->_crudCB.increaseOnce( ( op ), ( delta ) ) ;   \
      }                                                                       \
   }

#define DMS_MBSTAT_INC( _monAppCB_, _mbContext_, op, delta )                  \
   {                                                                          \
      if ( NULL != _monAppCB_ && NULL != _mbContext_ )                        \
      {                                                                       \
         _mbContext_->mbStat()->_crudCB.increase( ( op ), ( delta ) ) ;       \
      }                                                                       \
   }

}

#endif /* DMSSTORAGE_DATACOMMON_HPP_ */

