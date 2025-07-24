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

   Source File Name = dmsStorageBase.hpp

   Descriptive Name = Data Management Service Storage Unit Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/08/2013  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMSSTORAGE_BASE_HPP_
#define DMSSTORAGE_BASE_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossMmap.hpp"
#include "dms.hpp"
#include "dmsExtent.hpp"
#include "ossUtil.hpp"
#include "ossMem.hpp"
#include "../bson/bson.h"
#include "../bson/bsonobj.h"
#include "../bson/oid.h"
#include "dmsSMEMgr.hpp"
#include "dmsLobDef.hpp"


#include <string>

using namespace std ;
using namespace bson ;

namespace engine
{

   #define DMS_HEADER_EYECATCHER_LEN         (8)
   #define DMS_SU_NAME_SZ                    DMS_COLLECTION_SPACE_NAME_SZ

#pragma pack(4)
   /*
      _dmsStorageInfo defined
   */
   struct _dmsStorageInfo
   {
      SINT32      _pageSize ;
      CHAR        _suName [ DMS_SU_NAME_SZ + 1 ] ; // storage unit file name is
                                                   // foo.0 / foo.1, where foo
                                                   // is suName, and 0/1 are
                                                   // _sequence
      UINT32      _sequence ;
      UINT64      _secretValue ;
      INT32       _lobdPageSize ;

      UINT32      _overflowRatio ;
      UINT32      _extentThreshold ;

      _dmsStorageInfo ()
      {
         _pageSize      = DMS_PAGE_SIZE_DFT ;
         ossMemset( _suName, 0, sizeof( _suName ) ) ;
         _sequence      = 0 ;
         _secretValue   = 0 ;
         _lobdPageSize  = DMS_DO_NOT_CREATE_LOB ;

         _overflowRatio = 0 ;
         _extentThreshold = 0 ;
      }
   };
   typedef _dmsStorageInfo dmsStorageInfo ;

   /*
      Storage Unit Header : 65536(64K)
   */
   struct _dmsStorageUnitHeader : public SDBObject
   {
      CHAR   _eyeCatcher[DMS_HEADER_EYECATCHER_LEN] ;
      UINT32 _version ;
      UINT32 _pageSize ;                                 // size of byte
      UINT32 _storageUnitSize ;                          // all file pages
      CHAR   _name [ DMS_SU_NAME_SZ+1 ] ;                // storage unit name
      UINT32 _sequence ;                                 // storage unit seq
      UINT32 _numMB ;                                    // Number of MB
      UINT32 _MBHWM ;
      UINT32 _pageNum ;                                  // current page number
      UINT64 _secretValue ;                              // with the index
      UINT32 _lobdPageSize ;                             // lobd page size
      UINT32 _createLobs ;                               // create lob files
      UINT32 _validFlag ;                                // valid flag
      UINT64 _lsn ;
      CHAR   _pad [ 65344 ] ;

      _dmsStorageUnitHeader()
      {
         SDB_ASSERT( DMS_PAGE_SIZE_MAX == sizeof( _dmsStorageUnitHeader ),
                     "_dmsStorageUnitHeader size must be 64K" ) ;
         _lsn = -1 ;
         ossMemset( this, 0, DMS_PAGE_SIZE_MAX ) ;
      }
   } ;
   typedef _dmsStorageUnitHeader dmsStorageUnitHeader ;
   #define DMS_HEADER_SZ   sizeof(dmsStorageUnitHeader)

   #define DMS_SME_LEN                 (DMS_MAX_PG/8)
   #define DMS_SME_FREE                 0
   #define DMS_SME_ALLOCATED            1

   /* Space Management Extent, 1 bit for 1 page */
   struct _dmsSpaceManagementExtent : public SDBObject
   {
      CHAR _smeMask [ DMS_SME_LEN ] ;

      _dmsSpaceManagementExtent()
      {
         SDB_ASSERT( DMS_SME_LEN == sizeof( _dmsSpaceManagementExtent ),
                     "SME size error" ) ;
         ossMemset( _smeMask, DMS_SME_FREE, sizeof( _smeMask ) ) ;
      }
      CHAR getBitMask( UINT32 bitNum ) const
      {
         SDB_ASSERT( bitNum < DMS_MAX_PG, "Invalid bitNum" ) ;
         return (_smeMask[bitNum >> 3] >> (7 - (bitNum & 7))) & 1 ;
      }
      void freeBitMask( UINT32 bitNum )
      {
         SDB_ASSERT( bitNum < DMS_MAX_PG, "Invalid bitNum" ) ;
         _smeMask[bitNum >> 3] &= ~( 1 << (7 - (bitNum & 7))) ;
      }
      void setBitMask( UINT32 bitNum )
      {
         SDB_ASSERT( bitNum < DMS_MAX_PG, "Invalid bitNum" ) ;
         _smeMask[bitNum >> 3] |= ( 1 << (7 - (bitNum & 7))) ;
      }
   } ;
   typedef _dmsSpaceManagementExtent dmsSpaceManagementExtent ;
   #define DMS_SME_SZ  sizeof(dmsSpaceManagementExtent)

#pragma pack()

   void smeMask2String( CHAR state, CHAR *pBuffer, INT32 buffSize ) ;

   /*
      _dmsContext define
   */
   class _dmsContext : public SDBObject
   {
      public:
         _dmsContext () {}
         virtual ~_dmsContext () {}

      public:
         virtual string toString () const = 0 ;
         virtual INT32  pause () = 0 ;
         virtual INT32  resume () = 0 ;

   };
   typedef _dmsContext  dmsContext ;

   #define DMS_SU_FILENAME_SZ       ( DMS_SU_NAME_SZ + 15 )
   #define DMS_HEADER_OFFSET        ( 0 )
   #define DMS_SME_OFFSET           ( DMS_HEADER_OFFSET + DMS_HEADER_SZ )

   /*
      Storage Unit Base
   */
   class _dmsStorageBase : public ossMmapFile
   {
      friend class _dmsExtendSegmentJob ;
      typedef boost::shared_ptr<ossSpinSLatch>     sharedMutexPtr ;

      public:
         _dmsStorageBase( const CHAR *pSuFileName,
                          dmsStorageInfo *pInfo ) ;
         virtual ~_dmsStorageBase() ;

         const CHAR*    getSuFileName() const ;
         const CHAR*    getSuName() const ;
         const dmsStorageUnitHeader *getHeader() { return _dmsHeader ; }
         const dmsSpaceManagementExtent *getSME () { return _dmsSME ; }
         dmsSMEMgr *getSMEMgr () { return &_smeMgr ; }

         OSS_INLINE UINT64  dataSize () const ;
         OSS_INLINE UINT64  fileSize () const ;

         OSS_INLINE UINT32  pageSize () const ;
         OSS_INLINE UINT32  pageSizeSquareRoot () const ;
         OSS_INLINE UINT32  segmentPages () const ;
         OSS_INLINE UINT32  segmentPagesSquareRoot () const ;
         OSS_INLINE UINT32  maxSegmentNum() const ;
         OSS_INLINE UINT32  pageNum () const ;
         OSS_INLINE UINT32  freePageNum () const ;
         OSS_INLINE INT32   maxSegID () const ;
         OSS_INLINE UINT32  dataStartSegID () const ;
         OSS_INLINE BOOLEAN isTempSU () const { return _isTempSU ; }

         OSS_INLINE UINT32  extent2Segment( dmsExtentID extentID,
                                            UINT32 *pSegOffset = NULL ) ;
         OSS_INLINE dmsExtentID segment2Extent( UINT32 segID,
                                                UINT32 segOffset = 0 ) ;

         OSS_INLINE ossValuePtr extentAddr ( INT32 extentID ) ;
         OSS_INLINE dmsExtentID extentID ( ossValuePtr extendAddr ) ;
         // this function is used to prevent dmsStorageBase memory being freed
         // before page cleaner completes its job
         // the caller must perform the following steps BEFORE clean the pages
         // 1) suLock to lock the collection space, so that no one is able to
         // drop the collectionspace
         // 2) lock page cleaner, mark page cleaning is working
         // 3) suUnlock, so that other threads able to drop cs
         // 4) unlock page cleaner, once it's done
         // Since dmsStorageBase destructor will wait for latch before releasing
         // memory, so we should be safe here.
         // Note in page cleaning function, we check
         OSS_INLINE void lockPageCleaner () ;
         OSS_INLINE void unlockPageCleaner () ;

         OSS_INLINE UINT32 getLobdPageSize() const
         {
            return _lobPageSize ;
         }

         OSS_INLINE UINT64 getCurrentLSN() const
         {
            return _lastLSN.peek() ;
         }

         OSS_INLINE UINT32 getValidFlag() const
         {
            return _validFlag ;
         }

      public:
         INT32 openStorage ( const CHAR *pPath, BOOLEAN createNew = TRUE,
                             BOOLEAN delWhenExist = FALSE ) ;
         void  closeStorage () ;
         INT32 removeStorage() ;

         /// flush functions
         INT32 flushHeader( BOOLEAN sync = FALSE ) ;
         INT32 flushSME( BOOLEAN sync = FALSE ) ;
         INT32 flushMeta( BOOLEAN sync = FALSE,
                          UINT32 *pExceptID = NULL,
                          UINT32 num = 0 ) ;
         INT32 flushPages( SINT32 pageID, UINT16 pageNum,
                           BOOLEAN sync = FALSE ) ;
         INT32 flushSegment( UINT32 segmentID, BOOLEAN sync = FALSE ) ;
         INT32 flushAll( BOOLEAN sync = FALSE ) ;
         void  flushDirtySegments( UINT32 *pNum = NULL,
                                   BOOLEAN sync = TRUE ) ;

         void  restoreForCrash() { _isCrash = FALSE ; }
         void  resetLastLSN( UINT64 lsn ) { _lastLSN.init( lsn ) ; }
         UINT64 getLastTick() const { return _lastTick ; }

         /// virtual interface
         virtual void  syncMemToMmap () {}
         virtual INT32 tryToFlush( BOOLEAN ignoreTick, BOOLEAN &failed ) = 0 ;
         virtual BOOLEAN isOpened() const { return ossMmapFile::_opened ; }

      private:
         virtual UINT64 _dataOffset()  = 0 ;
         virtual const CHAR* _getEyeCatcher() const = 0 ;
         virtual UINT32 _curVersion() const = 0 ;
         virtual INT32  _checkVersion( dmsStorageUnitHeader *pHeader ) = 0 ;
         virtual INT32  _onCreate( OSSFILE *file, UINT64 curOffSet ) = 0 ;
         virtual INT32  _onMapMeta( UINT64 curOffSet ) = 0 ;
         virtual void   _onClosed() {}
         virtual INT32  _onOpened() { return SDB_OK ; }
         virtual UINT32 _extendThreshold() const ;
         virtual UINT32 _getSegmentSize() const ;
         virtual void   _initHeaderPageSize( dmsStorageUnitHeader *pHeader,
                                             dmsStorageInfo *pInfo ) ;
         virtual INT32  _checkPageSize( dmsStorageUnitHeader *pHeader ) ;
         virtual BOOLEAN _keepInRam() const { return FALSE ; }

         /// flush callback:  SDB_OK: continue, no SDB_OK: stop
         virtual INT32  _onFlushDirty( BOOLEAN sync ) { return SDB_OK ; }

      protected:
         // No space will extent new segment
         INT32    _findFreeSpace ( UINT16 numPages, SINT32 &foundPage,
                                   dmsContext *context ) ;
         INT32    _releaseSpace ( SINT32 pageStart, UINT16 numPages ) ;

         UINT32   _totalFreeSpace() ;

         INT32    _writeFile( OSSFILE *file, const CHAR *pData,
                              INT64 dataLen ) ;

         void     _markDirty ( INT32 extentID ) ;
         void     _markHeaderInvalid( BOOLEAN doublecheck ) ;
         void     _markHeaderValid() ;

         virtual INT32 _extendSegments ( UINT32 numSeg ) ;

         OSS_INLINE void _updateLastLSN( UINT64 offset )
         {
            if ( -1 != (INT64)offset )
            {
               _lastLSN.swapGreaterThan( offset ) ;
            }
            return ;
         }

         void     _registerNewWriting() ;
         BOOLEAN  _noWriteForAWhile() const ;

      private:
         INT32    _initializeStorageUnit () ;
         void     _initHeader ( dmsStorageUnitHeader *pHeader ) ;
         INT32    _validateHeader( dmsStorageUnitHeader *pHeader ) ;
         INT32    _preExtendSegment() ;

      protected:
         dmsStorageUnitHeader          *_dmsHeader ;     // 64KB
         dmsSpaceManagementExtent      *_dmsSME ;        // 8MB
         CHAR                          _suFileName[ DMS_SU_FILENAME_SZ + 1 ] ;

         dmsStorageInfo                *_pStorageInfo ;
         UINT32                        _pageSize ;    // cache, not use header
         UINT32                        _lobPageSize ; // cache, not use header

         CHAR                          *_dirtyList ;
         volatile UINT32               _validFlag ;
         volatile UINT64               _lastTick ;
         _ossAtomic64                  _lastLSN ;
         ossSpinXLatch                 _pagecleanerLatch ;

      private:
         sharedMutexPtr                _segmentLatch ;
         dmsSMEMgr                     _smeMgr ;
         UINT32                        _dataSegID ;
         UINT32                        _pageNum ;
         INT32                         _maxSegID ;
         UINT32                        _segmentPages ;
         UINT32                        _segmentPagesSquare ;
         UINT32                        _pageSizeSquare ;
         CHAR                          _fullPathName[ OSS_MAX_PATHSIZE + 1 ] ;
         BOOLEAN                       _isTempSU ;

         BOOLEAN                       _isCrash ;

   } ;
   typedef _dmsStorageBase dmsStorageBase ;

   /*
      _dmsStorageBase OSS_INLINE functions :
   */
   OSS_INLINE void _dmsStorageBase::lockPageCleaner ()
   {
      ossLatch ( &_pagecleanerLatch ) ;
   }
   OSS_INLINE void _dmsStorageBase::unlockPageCleaner ()
   {
      ossUnlatch ( &_pagecleanerLatch ) ;
   }
   OSS_INLINE UINT32 _dmsStorageBase::pageSize () const
   {
      return _pageSize ;
   }
   OSS_INLINE UINT32 _dmsStorageBase::pageSizeSquareRoot () const
   {
      return _pageSizeSquare ;
   }
   OSS_INLINE UINT32 _dmsStorageBase::segmentPages () const
   {
      return _segmentPages ;
   }
   OSS_INLINE UINT32 _dmsStorageBase::segmentPagesSquareRoot () const
   {
      return _segmentPagesSquare ;
   }
   OSS_INLINE UINT32 _dmsStorageBase::maxSegmentNum() const
   {
      return DMS_MAX_PG >> _segmentPagesSquare ;
   }
   OSS_INLINE UINT32 _dmsStorageBase::pageNum () const
   {
      return _pageNum ;
   }
   OSS_INLINE UINT32 _dmsStorageBase::freePageNum () const
   {
      return _smeMgr.totalFree() ;
   }
   OSS_INLINE INT32 _dmsStorageBase::maxSegID () const
   {
      return _maxSegID ;
   }
   OSS_INLINE UINT32 _dmsStorageBase::dataStartSegID () const
   {
      return _dataSegID ;
   }
   OSS_INLINE UINT64 _dmsStorageBase::dataSize () const
   {
      return (UINT64)_pageNum << _pageSizeSquare ;
   }
   OSS_INLINE UINT64 _dmsStorageBase::fileSize () const
   {
      if ( _dmsHeader )
      {
         return (UINT64)_dmsHeader->_storageUnitSize << _pageSizeSquare ;
      }
      return 0 ;
   }
   OSS_INLINE UINT32 _dmsStorageBase::extent2Segment( dmsExtentID extentID,
                                                      UINT32 * pSegOffset )
   {
      if ( pSegOffset )
      {
         // the same with : extentID % _segmentPages
         *pSegOffset = extentID & (( 1 << _segmentPagesSquare ) - 1 ) ;
      }
      // the same with: extentID / _segmentPages + _dataSegID
      return ( extentID >> _segmentPagesSquare ) + _dataSegID ;
   }
   OSS_INLINE dmsExtentID _dmsStorageBase::segment2Extent( UINT32 segID,
                                                           UINT32 segOffset )
   {
      if ( segID < _dataSegID )
      {
         return DMS_INVALID_EXTENT ;
      }
      // the same with: ( segID - _dataSegID ) * _segmentPages + segOffset
      return (( segID - _dataSegID ) << _segmentPagesSquare ) + segOffset ;
   }
   OSS_INLINE ossValuePtr _dmsStorageBase::extentAddr( INT32 extentID )
   {
      if ( DMS_INVALID_EXTENT == extentID )
      {
         return 0 ;
      }
      UINT32 segOffset = 0 ;
      UINT32 segID = extent2Segment( extentID, &segOffset ) ;
      if ( (INT32)segID > _maxSegID )
      {
         return 0 ;
      }
      return getSegmentInfo( segID ) +
             (ossValuePtr)( segOffset << _pageSizeSquare ) ;
      // the same with: segOffset * _segmentPages
   }
   OSS_INLINE dmsExtentID _dmsStorageBase::extentID( ossValuePtr extendAddr )
   {
      if ( 0 == extendAddr || _maxSegID < 0 )
      {
         return DMS_INVALID_EXTENT ;
      }
      // find seg ID
      INT32 segID = 0 ;
      UINT32 segOffset = 0 ;
      ossValuePtr tmpPtr = 0 ;
      UINT32 tmpLength = 0 ;
      while ( segID <= _maxSegID )
      {
         tmpPtr = getSegmentInfo( segID, &tmpLength ) ;
         if ( tmpPtr >= extendAddr &&
              extendAddr < tmpPtr + (ossValuePtr)tmpLength )
         {
            segOffset = (UINT32)((extendAddr - tmpPtr) >>
                                  _pageSizeSquare) ;
            break ;
         }
         ++segID ;
      }
      if ( segID > _maxSegID )
      {
         return DMS_INVALID_EXTENT ;
      }
      return segment2Extent( (UINT32)segID, segOffset ) ;
   }
   OSS_INLINE void _dmsStorageBase::_markDirty ( INT32 extentID )
   {
      // make sure the extentID is valid
      if ( DMS_INVALID_EXTENT == extentID ||
           extentID > DMS_MAX_PG )
         return ;
      //_markHeaderInvalid() ;
      // segment id is the real segment id minus the starting data segment id
      // position
      UINT32 segID = extent2Segment( extentID, NULL ) - _dataSegID ;
      SDB_ASSERT ( segID < maxSegmentNum(),
                   "calculated segment id cannot be greater than max "
                   "number of segments in the storage unit" ) ;
      // _dirtyList [ segID / 8 ] |= ( 1 << ( segID % 8 ) )
      _dirtyList [ segID >> 3 ] |= ( 1 << ( segID & 7 ) ) ;
   }

   /*
      DMS Other define
   */
   #define DMS_MON_OP_COUNT_INC( _pMonAppCB_, op, delta )  \
   {                                                       \
      if ( NULL != _pMonAppCB_ )                           \
      {                                                    \
         _pMonAppCB_->monOperationCountInc( op, delta ) ;  \
      }                                                    \
   }

   #define DMS_MON_OP_TIME_INC( _pMonAppCB_, op, delta )   \
   {                                                       \
      if ( NULL != _pMonAppCB_ )                           \
      {                                                    \
         _pMonAppCB_->monOperationTimeInc( op, delta ) ;   \
      }                                                    \
   }

   #define DMS_MON_CONTEXT_COUNT_INC( _monContextCB_, op, delta ) \
   {                                                               \
      if ( NULL != _monContextCB_ )                                \
      {                                                            \
         _monContextCB_->monOperationCountInc ( op, delta ) ;      \
      }                                                            \
   }

   #define DMS_MON_CONTEXT_TIME_INC( _monContextCB_, op, delta )  \
   {                                                               \
      if ( NULL != _monContextCB_ )                                \
      {                                                            \
         _monContextCB_->monOperationTimeInc ( op, delta ) ;       \
      }                                                            \
   }

   /****************************************************************************
    * Specify the matrix for collection flag and access type, returns TRUE means
    * access is allowed, otherwise return FALSE
    * AccessType:       Query  Fetch  Insert  Update  Delete  Truncate CRT-IDX  DROP-IDX
    *  FREE                N      N       N       N       N       N       N         N
    *  NORMAL              Y      Y       Y       Y       Y       Y       Y         Y
    *  DROPPED             N      N       N       N       N       N       N         N
    *  OFFLINE REORG       N (only alloed in shadow copy phase )
    *                             N       N       N       N       N ( only allowed in
    *  truncate phase )                                                   N         N
    *  ONLINE REORG        Y      Y       Y       Y       Y       Y       Y         Y
    *  Load                Y      Y       Y       Y       Y       N       Y         Y
    ***************************************************************************/
   BOOLEAN dmsAccessAndFlagCompatiblity ( UINT16 collectionFlag,
                                          DMS_ACCESS_TYPE accessType ) ;

}

#endif //DMSSTORAGE_BASE_HPP_

