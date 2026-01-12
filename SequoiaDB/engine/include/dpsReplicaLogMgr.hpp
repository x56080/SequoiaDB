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

   Source File Name = dpsReplicaLogMgr.hpp

   Descriptive Name = Data Protection Services Replica Log Manager Header

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for replica manager

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/27/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPSREPLICALOGMGR_H_
#define DPSREPLICALOGMGR_H_

#include "core.hpp"
#include "oss.hpp"
#include "dpsLogPage.hpp"
#include "monLatch.hpp"
#include "dpsMergeBlock.hpp"
#include "dpsTransCB.hpp"
#include "ossAtomic.hpp"
#include "dpsLogFileMgr.hpp"
#include "ossUtil.hpp"
#include "ossEvent.hpp"
#include "ossQueue.hpp"
#include "dpsMetaFile.hpp"
#include "utilCircularQueue.hpp"

#include <vector>
using namespace std ;

namespace engine
{

   // we ALWAYS search for MEM first, because we may have LSN stay in buffer
   // but not on disk
   #define  DPS_SEARCH_MEM       0x01
   // indicating also search in file
   #define  DPS_SEARCH_FILE      0x10
   #define  DPS_SEARCH_ALL       (DPS_SEARCH_MEM|DPS_SEARCH_FILE)

   #define DPS_INC_VER_ZERO      0
   #define DPS_INC_VER_DFT       1
   #define DPS_INC_VER_CRITICAL  10

   class _pmdEDUCB ;
   class dpsTransCB ;

   /*
      _dpsReplicaLogMgr define
   */
   class _dpsReplicaLogMgr : public SDBObject
   {
   private:
      typedef _utilCircularBuffer< _dpsLogPage * >       DPS_QUEUE_BUFFER ;
      typedef _utilCircularQueue< _dpsLogPage * >        DPS_QUEUE_CONTAINER ;
      typedef ossQueue< _dpsLogPage *, DPS_QUEUE_CONTAINER >
                                                         DPS_PAGE_QUEUE ;

      DPS_PAGE_QUEUE             *_queue ;
      DPS_QUEUE_BUFFER           _queueBuffer ;
      _dpsLogFileMgr             _logger;
      _dpsLogPage                *_pages;
      monSpinSLatch              _mtx ;
      monSpinXLatch              _writeMutex ;
      _ossAtomic32               _idleSize;
      DPS_LSN                    _lsn;
      DPS_LSN                    _currentLsn;
      DPS_LSN                    _lastCommitted ;
      UINT32                     _totalSize;
      UINT32                     _work;
      UINT32                     _begin ;
      BOOLEAN                    _rollFlag ;
      UINT32                     _pageNum;
      BOOLEAN                    _restoreFlag ;
      ossAutoEvent               _allocateEvent ;
      _ossAtomic32               _queSize ;

      dpsTransCB                 *_transCB ;
      vector< dpsEventHandler* > _vecEventHandler ;
      UINT8                      _incVerVal ;

      _dpsMetaFile               _metaFile ;

      UINT64                     _pageFlushCount ;
      DPS_LSN                    _pageFlushedBeginLSN ;
      UINT64                     _lastWriteMetaTick ;
      BOOLEAN                    _writeMetaPending ;

   public:
      _dpsReplicaLogMgr();

      ~_dpsReplicaLogMgr();

   public:
      OSS_INLINE UINT32 idleSize()
      {
         return _idleSize.peek();
      }

      OSS_INLINE DPS_LSN expectLsn()
      {
         DPS_LSN lsn ;
         _mtx.get_shared();
         lsn = _lsn;
         _mtx.release_shared();
         return lsn;
      }

      OSS_INLINE DPS_LSN tryExpectLsn()
      {
         DPS_LSN lsn ;

         if ( _mtx.try_get_shared() )
         {
            lsn = _lsn ;
            _mtx.release_shared();
         }

         return lsn;
      }

      OSS_INLINE DPS_LSN currentLsn()
      {
         DPS_LSN lsn ;
         _mtx.get_shared();
         lsn = _currentLsn;
         _mtx.release_shared();
         return lsn;
      }

      OSS_INLINE DPS_LSN commitLsn()
      {
         DPS_LSN lsn ;
         _mtx.get_shared() ;
         lsn = _lastCommitted ;
         _mtx.release_shared() ;
         return lsn ;
      }

      OSS_INLINE void incVersion( UINT8 incVerVal = DPS_INC_VER_DFT )
      {
         _mtx.get() ;
         if ( DPS_INVALID_LSN_VERSION == _lsn.version )
         {
            /// inc at now
            _lsn.version = DPS_INVALID_LSN_VERSION + incVerVal ;
            _incVerVal = DPS_INC_VER_ZERO ;
         }
         else
         {
            /// delay inc
            _incVerVal = incVerVal ;
         }
         _mtx.release() ;
      }

      OSS_INLINE void cancelIncVersion()
      {
         _mtx.get() ;
         _incVerVal = DPS_INC_VER_ZERO ;
         _mtx.release() ;
      }

      OSS_INLINE BOOLEAN hasDirty() const
      {
         return 0 !=_lastCommitted.compare( _currentLsn ) ? TRUE : FALSE ;
      }

      BOOLEAN isWriteMetaPending() const ;

      void regEventHandler( dpsEventHandler *pEventHandler ) ;
      void unregEventHandler( dpsEventHandler *pEventHandler ) ;

   public:
      DPS_LSN getStartLsn ( BOOLEAN logBufOnly ) ;
      void getLsnWindow( DPS_LSN &fileBeginLsn,
                         DPS_LSN &memBeginLsn,
                         DPS_LSN &endLsn,
                         DPS_LSN *expected,
                         DPS_LSN *committed ) ;

      INT32 init( const CHAR *path, UINT32 pageNum,
                  BOOLEAN enableSparse, dpsTransCB *pTransCB ) ;
      void  fini() ;

      INT32 merge( _dpsMergeBlock &block ) ;

      // first step: allocate pages and product lsn
      INT32 preparePages ( dpsMergeInfo &info ) ;
      // secondary step: write data to pages
      void  writeData ( dpsMergeInfo &info ) ;

      INT32 search( const DPS_LSN &minLsn, _dpsMessageBlock *mb,
                    UINT8 type, BOOLEAN onlyHeader,
                    UINT32 *pLength = NULL );
      INT32 run( _pmdEDUCB *cb );
      INT32 tearDown();
      INT32 flushAll() ;

      /// committedLsn should be allocated by user
      INT32 commit( BOOLEAN deeply, DPS_LSN *committedLsn, BOOLEAN updateMeta = FALSE ) ;

      INT32 checkSyncControl( UINT32 reqLen, _pmdEDUCB *cb ) ;

      INT32 checkSeondarySyncControl( UINT32 reqLen, _pmdEDUCB *cb ) ;

      /// any other interfaces should not be called
      /// when this interface is beging called.
      INT32 move( const DPS_LSN_OFFSET &offset,
                  const DPS_LSN_VER &version ) ;

      void setLogFileSz ( UINT64 logFileSz )
      {
         _logger.setLogFileSz ( logFileSz ) ;
      }
      UINT32 getLogFileSz ()
      {
         return _logger.getLogFileSz () ;
      }
      void setLogFileNum ( UINT32 logFileNum )
      {
         _logger.setLogFileNum ( logFileNum ) ;
      }
      UINT32 getLogFileNum ()
      {
         return _logger.getLogFileNum () ;
      }
      void setFsCacheExpiredMs( UINT64 fsCacheExpiredMs )
      {
         _logger.setFsCacheExpiredMs( fsCacheExpiredMs ) ;
      }

      BOOLEAN isInRestore ()
      {
         return _restoreFlag;
      }

      _dpsLogFile* getLogFile( UINT32 fileId )
      {
         return _logger.getLogFile( fileId ) ;
      }

      _dpsLogFile* getLogFileByLogicalID( UINT32 logicalFileId )
      {
         return _logger.getLogFile( logicalFileId % getLogFileNum() ) ;
      }

      UINT32 calcFileID ( DPS_LSN_OFFSET offset )
      {
         return ( offset / getLogFileSz () ) % getLogFileNum () ;
      }

      UINT32 calcLogicalFileID( DPS_LSN_OFFSET offset )
      {
         return ( offset / getLogFileSz () ) ;
      }

      BOOLEAN isFirstPhysicalLSNOfFile( DPS_LSN_OFFSET offset )
      {
         return ( ( offset % getLogFileSz () ) == 0 ) ? TRUE : FALSE ;
      }

      DPS_LSN_OFFSET calcFirstPhysicalLSNOfFile( UINT32 logicalFileId )
      {
         // file id start from 0
         return ((UINT64)logicalFileId) * getLogFileSz () ;
      }

      DPS_LSN_OFFSET getFirstLSNOfFile( UINT32 fileId )
      {
         DPS_LSN_OFFSET offset = DPS_INVALID_LSN_OFFSET ;
         dpsLogFile* file = getLogFile( fileId ) ;
         if ( file )
         {
            offset = file->getFirstLSN( FALSE ).offset ;
         }
         return offset ;
      }

      DPS_LSN_OFFSET readOldestBeginLsnOffset() const ;

      UINT32 getLoggerLogicalWork ()
      {
         return _logger.getLogicalWorkPos() ;
      }

      BOOLEAN canInvalidateFsCache() const
      {
         return _logger.canInvalidateFsCache() ;
      }

      INT32 invalidateFsCache( const UINT64 *pExpiredMs = NULL )
      {
         return _logger.invalidateFsCache( pExpiredMs ) ;
      }

      const dpsMetaFileContent& getMetaContent() const
      {
         return _metaFile.getContent() ;
      }

   private:
      void _allocate( UINT32 len,
                      dpsPageMeta &allocated ) ;
      void _push2SendQueue( const dpsPageMeta &allocated );
      void _mergeLogs( _dpsMergeBlock &block,
                       const dpsPageMeta &meta ) ;
      void _mergePage( const CHAR *src,
                       UINT32 len,
                       UINT32 &workSub,
                       UINT32 &offset );
      INT32 _flushPage( _dpsLogPage *page, BOOLEAN shutdown = FALSE );
      INT32 _flushAll() ;
      INT32 _search ( const DPS_LSN &lsn, _dpsMessageBlock *mb,
                      BOOLEAN onlyHeader,
                      UINT32 *pLength = NULL ) ;
      DPS_LSN _getStartLsn () ;
      INT32 _parse( UINT32 sub, UINT32 offset, UINT32 len, CHAR *out ) ;

      INT32  _movePages ( const DPS_LSN_OFFSET &offset,
                          const DPS_LSN_VER &version ) ;
      INT32 _restore ( const DPS_LSN &fastStartLsn = DPS_LSN() ) ;

      INT32 _restoreMeta( BOOLEAN metaContentValid ) ;

      UINT32 _decPageID ( UINT32 pageID )
      {
         return pageID ? pageID - 1 : _pageNum - 1 ;
      }
      UINT32 _incPageID ( UINT32 pageID )
      {
         ++pageID ;
         return pageID >= _pageNum ? 0 : pageID ;
      }

      void _flushOldestTransBeginLSN() ;

      UINT32 _generateDummySize( dpsMergeBlock &block,
                                 dpsLogRecordHeader &head,
                                 UINT32 logFileSz ) ;
   };
   typedef class _dpsReplicaLogMgr dpsReplicaLogMgr;
}

#endif //DPSREPLICALOGMGR_H_

