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

   Source File Name = pmdEDU.cpp

   Descriptive Name = Process MoDel Agent Engine Dispatchable Unit

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for EDU processing.
   EDU thread is a wrapper of all threads. It will call each entry function
   depends on the EDU type.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include <stdio.h>
#include "pd.hpp"
#include "ossMem.hpp"
#include "pmdEDU.hpp"
#include "pmdEntryPoint.hpp"
#include "pmd.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"
#include "utilUniqueID.hpp"
#include <map>

namespace engine
{
   const UINT32 EDU_MEM_ALIGMENT_SIZE  = 1024 ; // must for times for 4
   const UINT32 EDU_MAX_CATCH_SIZE     = 16*1024*1024 ;

   /*
      TOOL FUNCTIONS
   */
   const CHAR * getEDUStatusDesp ( EDU_STATUS status )
   {
      const CHAR *desp = "Unknown" ;

      switch ( status )
      {
         case PMD_EDU_CREATING :
            desp = "Creating" ;
            break ;
         case PMD_EDU_RUNNING :
            desp = "Running" ;
            break ;
         case PMD_EDU_WAITING :
            desp = "Waiting" ;
            break ;
         case PMD_EDU_IDLE :
            desp = "Idle" ;
            break ;
         default :
            break ;
      }

      return desp ;
   }

   /*
      _pmdEDUCB implement
   */
   _pmdEDUCB::_pmdEDUCB( _pmdEDUMgr *mgr, INT32 type )
#if defined ( SDB_ENGINE )
   :_transExecutor( this )
#endif // SDB_ENGINE
   {
      _eduMgr           = mgr ;
      _eduID            = PMD_INVALID_EDUID ;
      _tid              = 0 ;
      _status           = PMD_EDU_UNKNOW ;
      _eduType          = type ;
      _isLocked         = FALSE ;
      _ctrlFlag         = 0 ;
      _isInterruptSelf  = FALSE ;
      _writingDB        = FALSE ;
      _writingID        = 0 ;
      _processEventCount= 0 ;
      ossMemset( _name, 0, sizeof( _name ) ) ;
      ossMemset( _source, 0, sizeof( _source ) ) ;
      _threadHdl        = 0 ;
      _pSession         = NULL ;
      _pRemoteSite      = NULL ;
      _pCompressBuff    = NULL ;
      _compressBuffLen  = 0 ;
      _pUncompressBuff  = NULL ;
      _uncompressBuffLen= 0 ;
      _totalCatchSize   = 0 ;
      _totalMemSize     = 0 ;
      _isDoRollback     = FALSE ;
      _pClientSock      = NULL ;

      _alignedMem       = NULL ;
      _alignedMemSize   = 0 ;

      _pBuffer          = NULL ;
      _buffSize         = 0 ;

      _beginLsn         = 0 ;
      _endLsn           = 0 ;
      _lsnNumber        = 0 ;
      _doRollback       = FALSE ;

      _curTransLSN      = DPS_INVALID_LSN_OFFSET ;
      _curTransID       = DPS_INVALID_TRANS_ID ;

#if defined (_LINUX)
      _threadID         = 0 ;
#endif // _LINUX

#if defined ( SDB_ENGINE )
      _relatedTransLSN  = DPS_INVALID_LSN_OFFSET ;
      _transRC          = SDB_OK ;

      _curRequestID     = 1 ;
      _confChangeID     = 0 ;
#endif // SDB_ENGINE

      _pErrorBuff = (CHAR *)SDB_OSS_MALLOC( EDU_ERROR_BUFF_SIZE + 1 ) ;
      if ( _pErrorBuff )
      {
         ossMemset( _pErrorBuff, 0, EDU_ERROR_BUFF_SIZE + 1 ) ;
      }

   }

   _pmdEDUCB::~_pmdEDUCB ()
   {
      if ( _pErrorBuff )
      {
         SDB_OSS_FREE ( _pErrorBuff ) ;
         _pErrorBuff = NULL ;
      }

      clear() ;
   }

   void _pmdEDUCB::clear()
   {
      // clear all queue msg
      pmdEDUEvent data ;
      while ( _queue.try_pop( data ) )
      {
         pmdEduEventRelase( data, this ) ;
      }
      _processEventCount = 0 ;
      ossMemset( _name, 0, sizeof( _name ) ) ;
      ossMemset( _source, 0, sizeof( _source ) ) ;
      _userName = "" ;
      _passWord = "" ;
      _isLocked = FALSE ;

      _ctrlFlag = 0 ;
      _isInterruptSelf = FALSE ;
      resetLsn() ;
      writingDB( FALSE ) ;
      releaseAlignedBuff() ;
      releaseBuffer() ;

      resetLocks() ;

#if defined ( SDB_ENGINE )
      clearTransInfo() ;
#endif // SDB_ENGINE

      // release buff
      if ( _pCompressBuff )
      {
         releaseBuff( _pCompressBuff ) ;
         _pCompressBuff = NULL ;
      }
      _compressBuffLen = 0 ;
      if ( _pUncompressBuff )
      {
         releaseBuff( _pUncompressBuff ) ;
         _pUncompressBuff = NULL ;
      }
      _uncompressBuffLen = 0 ;

      // clean catch
      CATCH_MAP_IT it = _catchMap.begin() ;
      while ( it != _catchMap.end() )
      {
         SDB_OSS_FREE( it->second ) ;
         _totalCatchSize -= it->first ;
         _totalMemSize -= it->first ;
         ++it ;
      }
      _catchMap.clear() ;

      // clean alloc memory
      ALLOC_MAP_IT itAlloc = _allocMap.begin() ;
      while ( itAlloc != _allocMap.end() )
      {
         SDB_OSS_FREE( itAlloc->first ) ;
         _totalMemSize -= itAlloc->second ;
         ++itAlloc ;
      }
      _allocMap.clear() ;

      SDB_ASSERT( _totalCatchSize == 0 , "Catch size is error" ) ;
      SDB_ASSERT( _totalMemSize == 0, "Memory size is error" ) ;

   }

   string _pmdEDUCB::toString() const
   {
      stringstream ss ;
      ss << "ID: " << _eduID << ", Type: " << _eduType << "["
         << getEDUName( _eduType ) << "], TID: " << _tid ;

      if ( _pSession )
      {
         ss << ", Session: " << _pSession->sessionName() ;
      }

      return ss.str() ;
   }

   const CHAR* _pmdEDUCB::getName () const
   {
      return _name ;
   }

   const CHAR* _pmdEDUCB::getSource() const
   {
      return _source ;
   }

   void _pmdEDUCB::attachSession( ISession *pSession )
   {
      _pSession = pSession ;
   }

   void _pmdEDUCB::detachSession()
   {
      ossScopedLock lock( &_mutex, EXCLUSIVE ) ;
      _pSession = NULL ;
   }

   void _pmdEDUCB::attachRemoteSite( IRemoteSite *pSite )
   {
      _pRemoteSite = pSite ;
   }

   void _pmdEDUCB::detachRemoteSite()
   {
      _pRemoteSite = NULL ;
   }

   void _pmdEDUCB::setType ( INT32 type )
   {
      _eduType = type ;
   }

   void _pmdEDUCB::interrupt( BOOLEAN onlySelf )
   {
      _ctrlFlag |= EDU_CTRL_INTERRUPTED ;
      _isInterruptSelf = onlySelf ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUCB_DISCONNECT, "_pmdEDUCB::disconnect" )
   void _pmdEDUCB::disconnect ()
   {
      PD_TRACE_ENTRY ( SDB__PMDEDUCB_DISCONNECT );
      interrupt () ;
      _ctrlFlag |= EDU_CTRL_DISCONNECTED ;
      postEvent ( pmdEDUEvent ( PMD_EDU_EVENT_TERM ) ) ;
      PD_TRACE_EXIT ( SDB__PMDEDUCB_DISCONNECT );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUCB_FORCE, "_pmdEDUCB::force" )
   void _pmdEDUCB::force ()
   {
      PD_TRACE_ENTRY ( SDB__PMDEDUCB_FORCE );
      disconnect () ;
      _ctrlFlag |= EDU_CTRL_FORCED ;
      PD_TRACE_EXIT ( SDB__PMDEDUCB_FORCE );
   }

   void _pmdEDUCB::resetInterrupt ()
   {
      _ctrlFlag &= ~EDU_CTRL_INTERRUPTED ;
      _isInterruptSelf = FALSE ;
   }

   void _pmdEDUCB::resetDisconnect ()
   {
      resetInterrupt () ;
      _ctrlFlag &= ~EDU_CTRL_DISCONNECTED ;
   }

   void _pmdEDUCB::setUserInfo( const string & userName,
                                const string & password )
   {
      _userName = userName ;
      _passWord = password ;
   }

   void _pmdEDUCB::setName ( const CHAR * name )
   {
      ossStrncpy ( _name, name, PMD_EDU_NAME_LENGTH ) ;
      _name[PMD_EDU_NAME_LENGTH] = 0 ;
   }

   void _pmdEDUCB::setSource( const CHAR *pSource )
   {
      ossStrncpy( _source, pSource, PMD_EDU_NAME_LENGTH ) ;
      _source[ PMD_EDU_NAME_LENGTH ] = 0 ;
   }

   CHAR *_pmdEDUCB::_getBuffInfo ( EDU_INFO_TYPE type, UINT32 & size )
   {
      CHAR *buff = NULL ;
      switch ( type )
      {
         case EDU_INFO_ERROR :
            buff = _pErrorBuff ;
            size = EDU_ERROR_BUFF_SIZE ;
            break ;
         default :
            break ;
      }

      return buff ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUCB_PRINTINFO, "_pmdEDUCB::printInfo" )
   INT32 _pmdEDUCB::printInfo ( EDU_INFO_TYPE type, const CHAR * format, ... )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__PMDEDUCB_PRINTINFO );
      //already exist, return ok
      if ( getInfo ( type ) )
      {
         goto done ;
      }

      {
      UINT32 buffSize = 0 ;
      CHAR *buff = _getBuffInfo ( type, buffSize ) ;

      if ( NULL == buff || buffSize == 0 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      va_list ap ;
      va_start ( ap, format ) ;
      vsnprintf ( buff, buffSize, format, ap ) ;
      va_end ( ap ) ;

      buff[ buffSize ] = 0 ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__PMDEDUCB_PRINTINFO, rc );
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUCB_GETINFO, "_pmdEDUCB::getInfo" )
   const CHAR *_pmdEDUCB::getInfo ( EDU_INFO_TYPE type )
   {
      PD_TRACE_ENTRY ( SDB__PMDEDUCB_GETINFO );
      UINT32 buffSize = 0 ;
      CHAR *buff = _getBuffInfo ( type, buffSize ) ;
      if ( buff && buff[0] != 0 )
      {
         PD_TRACE_EXIT ( SDB__PMDEDUCB_GETINFO );
         return buff ;
      }
      PD_TRACE_EXIT ( SDB__PMDEDUCB_GETINFO );
      return NULL ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUCB_RESETINFO, "_pmdEDUCB::resetInfo" )
   void _pmdEDUCB::resetInfo ( EDU_INFO_TYPE type )
   {
      PD_TRACE_ENTRY ( SDB__PMDEDUCB_RESETINFO );
      UINT32 buffSize = 0 ;
      CHAR *buff = _getBuffInfo ( type, buffSize ) ;
      if ( buff )
      {
         buff[0] = 0 ;
      }
      PD_TRACE_EXIT ( SDB__PMDEDUCB_RESETINFO );
   }

   BOOLEAN _pmdEDUCB::_allocFromCatch( UINT32 len, CHAR **ppBuff,
                                       UINT32 *buffLen )
   {
      UINT32 tmpLen = 0 ;
      CATCH_MAP_IT it = _catchMap.lower_bound( len ) ;
      if ( it != _catchMap.end() )
      {
         *ppBuff = it->second ;
         tmpLen = it->first ;
         _catchMap.erase( it ) ;
         _allocMap[ *ppBuff ] = tmpLen ;
         _totalCatchSize -= tmpLen ;

         if ( buffLen )
         {
            *buffLen = tmpLen ;
         }
         return TRUE ;
      }
      return FALSE ;
   }

   void _pmdEDUCB::restoreBuffs( _pmdEDUCB::CATCH_MAP &catchMap )
   {
      CATCH_MAP_IT it = catchMap.begin() ;
      while ( it != catchMap.end() )
      {
         _catchMap.insert( std::make_pair( it->first, it->second ) ) ;
         _totalCatchSize += it->first ;
         _totalMemSize += it->first ;
         ++it ;
      }
      catchMap.clear() ;
   }

   void _pmdEDUCB::saveBuffs( _pmdEDUCB::CATCH_MAP &catchMap )
   {
      // release buff
      if ( _pCompressBuff )
      {
         releaseBuff( _pCompressBuff ) ;
         _pCompressBuff = NULL ;
      }
      _compressBuffLen = 0 ;
      if ( _pUncompressBuff )
      {
         releaseBuff( _pUncompressBuff ) ;
         _pUncompressBuff = NULL ;
      }
      _uncompressBuffLen = 0 ;

      // clean alloc memory
      CHAR *pBuff = NULL ;
      ALLOC_MAP_IT itAlloc = _allocMap.begin() ;
      while ( itAlloc != _allocMap.end() )
      {
         pBuff = itAlloc->first ;
         ++itAlloc ;
         releaseBuff( pBuff ) ;
      }
      _allocMap.clear() ;

      // restore catch map
      CATCH_MAP_IT it = _catchMap.begin() ;
      while ( it != _catchMap.end() )
      {
         _totalCatchSize -= it->first ;
         _totalMemSize -= it->first ;
         catchMap.insert( std::make_pair( it->first, it->second ) ) ;
         ++it ;
      }
      _catchMap.clear() ;
   }

   CHAR* _pmdEDUCB::getCompressBuff( UINT32 len )
   {
      if ( _compressBuffLen < len )
      {
         if ( _pCompressBuff )
         {
            releaseBuff( _pCompressBuff ) ;
            _pCompressBuff = NULL ;
         }
         _compressBuffLen = 0 ;

         allocBuff( len, &_pCompressBuff, &_compressBuffLen ) ;
      }

      return _pCompressBuff ;
   }

   CHAR* _pmdEDUCB::getUncompressBuff( UINT32 len )
   {
      if ( _uncompressBuffLen < len )
      {
         if ( _pUncompressBuff )
         {
            releaseBuff( _pUncompressBuff ) ;
            _pUncompressBuff = NULL ;
         }
         _uncompressBuffLen = 0 ;

         allocBuff( len, &_pUncompressBuff, &_uncompressBuffLen ) ;
      }

      return _pUncompressBuff ;
   }

   /*
      Interface impelent
   */
   void *_pmdEDUCB::getAlignedBuff( UINT32 size,
                                    UINT32 *pRealSize,
                                    UINT32 alignment )
   {
      SDB_ASSERT( alignment == OSS_FILE_DIRECT_IO_ALIGNMENT,
                  "rewrite this function if u want to use new alignment" ) ;
      if ( _alignedMemSize < size )
      {
         if ( NULL != _alignedMem )
         {
            SDB_OSS_ORIGINAL_FREE( _alignedMem ) ;
            _alignedMemSize = 0 ;
            _alignedMem = NULL ;
         }

         size = ossRoundUpToMultipleX( size, alignment ) ;
         _alignedMem = ossAlignedAlloc( alignment, size ) ;
         if ( NULL != _alignedMem )
         {
            _alignedMemSize = size ;
         }
      }

      if ( pRealSize )
      {
         *pRealSize = _alignedMemSize ;
      }

      return _alignedMem ;
   }

   void _pmdEDUCB::releaseAlignedBuff()
   {
      if ( NULL != _alignedMem )
      {
         SDB_OSS_ORIGINAL_FREE( _alignedMem ) ;
         _alignedMem = NULL ;
         _alignedMemSize = 0 ;
      }
   }

   CHAR* _pmdEDUCB::getBuffer( UINT32 len )
   {
      if ( _buffSize < len )
      {
         releaseBuffer() ;
         allocBuff( len, &_pBuffer, &_buffSize ) ;
      }

      return _pBuffer ;
   }

   void _pmdEDUCB::releaseBuffer()
   {
      if ( _pBuffer )
      {
         releaseBuff( _pBuffer ) ;
         _pBuffer = NULL ;
         _buffSize = 0 ;
      }
   }

   INT32 _pmdEDUCB::allocBuff( UINT32 len,
                               CHAR **ppBuff,
                               UINT32 *pRealSize )
   {
      INT32 rc = SDB_OK ;

      // first alloc from catch
      if ( _totalCatchSize >= len &&
           _allocFromCatch( len, ppBuff, pRealSize ) )
      {
         goto done ;
      }

      // malloc
      len = ossRoundUpToMultipleX( len, EDU_MEM_ALIGMENT_SIZE ) ;
      *ppBuff = ( CHAR* )SDB_OSS_MALLOC( len ) ;
      if( !*ppBuff )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Edu[%s] malloc memory[size: %u] failed",
                 toString().c_str(), len ) ;
         goto error ;
      }

      // update meta info
      _totalMemSize += len ;
      _allocMap[ *ppBuff ] = len ;

      if ( pRealSize )
      {
         *pRealSize = len ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _pmdEDUCB::reallocBuff( UINT32 len,
                                 CHAR **ppBuff,
                                 UINT32 *pRealSize )
   {
      INT32 rc = SDB_OK ;
      CHAR *pOld = *ppBuff ;
      UINT32 oldLen = 0 ;

      ALLOC_MAP_IT itAlloc = _allocMap.find( *ppBuff ) ;
      if ( itAlloc != _allocMap.end() )
      {
         oldLen = itAlloc->second ;
         if ( pRealSize )
         {
            *pRealSize = oldLen ;
         }
      }
      else if ( *ppBuff != NULL )
      {
         PD_LOG( PDERROR, "EDU[%s] realloc input buffer error",
                 toString().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( oldLen >= len )
      {
         goto done ;
      }
      len = ossRoundUpToMultipleX( len, EDU_MEM_ALIGMENT_SIZE ) ;
      *ppBuff = ( CHAR* )SDB_OSS_REALLOC( *ppBuff, len ) ;
      if ( !*ppBuff )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to realloc memory, size: %d", len ) ;
         goto error ;
      }

      if ( pOld != *ppBuff )
      {
         /// the old pointer has release, so need del from map
         _allocMap.erase( pOld ) ;
      }

      // update meta info
      _totalMemSize += ( len - oldLen ) ;

      _allocMap[ *ppBuff ] = len ;

      if ( pRealSize )
      {
         *pRealSize = len ;
      }

   done:
      return rc ;
   error:
      if ( pOld )
      {
         releaseBuff( pOld ) ;
         *ppBuff = NULL ;
         if ( pRealSize )
         {
            *pRealSize = 0 ;
         }
      }
      goto done ;
   }

   void _pmdEDUCB::releaseBuff( CHAR *pBuff )
   {
      ALLOC_MAP_IT itAlloc = _allocMap.find( pBuff ) ;
      if ( itAlloc == _allocMap.end() )
      {
         SDB_OSS_FREE( pBuff ) ;
         return ;
      }
      INT32 buffLen = itAlloc->second ;
      _allocMap.erase( itAlloc ) ;

      if ( (UINT32)buffLen > EDU_MAX_CATCH_SIZE )
      {
         SDB_OSS_FREE( pBuff ) ;
         _totalMemSize -= buffLen ;
      }
      else
      {
         // add to catch
         _catchMap.insert( std::make_pair( buffLen, pBuff ) ) ;
         _totalCatchSize += buffLen ;

         // re-org catch
         while ( _totalCatchSize > EDU_MAX_CATCH_SIZE )
         {
            CATCH_MAP_IT it = _catchMap.begin() ;
            SDB_OSS_FREE( it->second ) ;
            _totalMemSize -= it->first ;
            _totalCatchSize -= it->first ;
            _catchMap.erase( it ) ;
         }
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUCB_ISINT, "_pmdEDUCB::isInterrupted" )
   BOOLEAN _pmdEDUCB::isInterrupted ( BOOLEAN onlyFlag )
   {
      PD_TRACE_ENTRY ( SDB__PMDEDUCB_ISINT );
      BOOLEAN ret = FALSE ;

      // mask interrupt while doing rollback
      if ( !onlyFlag && _isDoRollback )
      {
         goto done;
      }
      if ( _ctrlFlag & EDU_CTRL_INTERRUPTED )
      {
         ret = TRUE ;
         goto done ;
      }
      else if ( !onlyFlag && _pClientSock )
      {
         if ( _pClientSock->isClosed() )
         {
            _ctrlFlag |= ( EDU_CTRL_INTERRUPTED | EDU_CTRL_DISCONNECTED ) ;
            _isInterruptSelf = FALSE ;
            ret = TRUE ;
         }
         else
         {
            INT32 receivedLen ;
            MsgHeader header ;
            INT32 rc = _pClientSock->recv( (CHAR*)&header , sizeof(header),
                                           receivedLen, 0, MSG_PEEK, TRUE, TRUE ) ;
            if ( ( rc >= (INT32)sizeof(header) &&
                   MSG_BS_DISCONNECT == header.opCode ) ||
                 SDB_NETWORK_CLOSE == rc ||
                 SDB_NETWORK == rc )
            {
               _ctrlFlag |= ( EDU_CTRL_INTERRUPTED | EDU_CTRL_DISCONNECTED ) ;
               _isInterruptSelf = FALSE ;
               ret = TRUE ;
            }
            else if ( rc >= (INT32)sizeof(header) &&
                      ( MSG_BS_INTERRUPTE == header.opCode ||
                        MSG_BS_INTERRUPTE_SELF == header.opCode ) )
            {
               _ctrlFlag |= EDU_CTRL_INTERRUPTED ;
               _isInterruptSelf = MSG_BS_INTERRUPTE_SELF == header.opCode ?
                                  TRUE : FALSE ;
               ret = TRUE ;
            }
         }
      }
   done :
      PD_TRACE1 ( SDB__PMDEDUCB_ISINT, PD_PACK_INT(ret) );
      PD_TRACE_EXIT ( SDB__PMDEDUCB_ISINT );
      return ret ;
   }

   BOOLEAN _pmdEDUCB::isOnlySelfWhenInterrupt() const
   {
      return _isInterruptSelf ;
   }

   BOOLEAN _pmdEDUCB::isDisconnected ()
   {
      return ( _ctrlFlag & EDU_CTRL_DISCONNECTED ) ? TRUE : FALSE ;
   }

   BOOLEAN _pmdEDUCB::isForced ()
   {
      return ( _ctrlFlag & EDU_CTRL_FORCED ) ? TRUE : FALSE ;
   }

   void _pmdEDUCB::writingDB( BOOLEAN writing )
   {
      if ( _writingDB == writing ) return ;

      if ( writing )
      {
         _writingDB = TRUE ;
         _writingID = pmdAcquireGlobalID() ;
      }
      else if ( !writing && 0 == getLockItem(SDB_LOCK_DMS)->lockCount() &&
                ( DPS_INVALID_TRANS_ID == _curTransID ||
                  DPS_INVALID_LSN_OFFSET == _curTransLSN ) )
      {
         _writingDB = FALSE ;
         _writingID = 0 ;
      }
   }

   void _pmdEDUCB::resetLsn()
   {
      _beginLsn = ~0 ;
      _endLsn = ~0 ;
      _lsnNumber = 0 ;
      _doRollback = FALSE ;
   }

   void _pmdEDUCB::insertLsn( UINT64 lsn, BOOLEAN isRollback )
   {
      if ( _beginLsn == (UINT64)~0 )
      {
         _beginLsn = lsn ;
      }
      _endLsn = lsn ;
      _lsnNumber++ ;
      _doRollback = isRollback ;
   }

   void _pmdEDUCB::setTransID( UINT64 transID )
   {
      _curTransID = transID ;
   }

   void _pmdEDUCB::setCurTransLsn( UINT64 lsn )
   {
      _curTransLSN = lsn ;
   }

   void _pmdEDUCB::contextInsert( INT64 contextID )
   {
      ossScopedLock _lock ( &_mutex, EXCLUSIVE ) ;
      _contextList.insert ( contextID ) ;
   }

   void _pmdEDUCB::contextDelete( INT64 contextID )
   {
      ossScopedLock _lock ( &_mutex, EXCLUSIVE ) ;
      _contextList.erase ( contextID ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDEDUCB_CONTXTPEEK, "_pmdEDUCB::contextPeek" )
   INT64 _pmdEDUCB::contextPeek()
   {
      PD_TRACE_ENTRY ( SDB__PMDEDUCB_CONTXTPEEK );
      ossScopedLock _lock ( &_mutex, EXCLUSIVE ) ;
      SINT64 contextID = -1 ;
      SET_CONTEXT::const_iterator it ;
      if ( _contextList.empty() )
      {
         goto done ;
      }
      it = _contextList.begin() ;
      contextID = (*it) ;
      _contextList.erase(it) ;

   done :
      PD_TRACE1 ( SDB__PMDEDUCB_CONTXTPEEK, PD_PACK_LONG(contextID) );
      PD_TRACE_EXIT ( SDB__PMDEDUCB_CONTXTPEEK );
      return contextID ;
   }

   BOOLEAN _pmdEDUCB::contextFind( INT64 contextID )
   {
      ossScopedLock _lock ( &_mutex, SHARED ) ;
      return _contextList.end() != _contextList.find( contextID ) ;
   }

   UINT32 _pmdEDUCB::contextNum()
   {
      ossScopedLock _lock ( &_mutex, SHARED ) ;
      return _contextList.size() ;
   }

   void _pmdEDUCB::contextCopy( _pmdEDUCB::SET_CONTEXT &contextList )
   {
      ossScopedLock _lock ( &_mutex, SHARED ) ;
      contextList = _contextList ;
   }

   void _pmdEDUCB::initMonAppCB()
   {
      _monApplCB.reset() ;
      if ( _monCfgCB.timestampON )
      {
         _monApplCB.recordConnectTimestamp() ;
      }
   }

   void _pmdEDUCB::initTransConf()
   {
#if defined ( SDB_ENGINE )
      pmdOptionsCB *optCB = pmdGetOptionCB() ;
      if ( optCB->transactionOn() )
      {
         _confChangeID = optCB->getChangeID() ;
         _transExecutor.initTransConf( optCB->transIsolation(),
                                       optCB->transTimeout() * OSS_ONE_SEC,
                                       optCB->transLockwait(),
                                       optCB->transAutoCommit(),
                                       optCB->transAutoRollback(),
                                       optCB->transUseRBS() ) ;
      }
#endif //SDB_ENGINE
   }

   void _pmdEDUCB::updateTransConf()
   {
#if defined ( SDB_ENGINE )
      pmdOptionsCB *optCB = pmdGetOptionCB() ;
      if ( optCB->transactionOn() && _confChangeID != optCB->getChangeID() )
      {
         if ( _transExecutor.updateTransConf( optCB->transIsolation(),
                                              optCB->transTimeout() * OSS_ONE_SEC,
                                              optCB->transLockwait(),
                                              optCB->transAutoCommit(),
                                              optCB->transAutoRollback(),
                                              optCB->transUseRBS() ) )
         {
            _confChangeID = optCB->getChangeID() ;
         }
      }
#endif //SDB_ENGINE
   }

   void _pmdEDUCB::incEventCount( UINT32 step )
   {
      _processEventCount += step ;
   }

   UINT32 _pmdEDUCB::getQueSize()
   {
      return _queue.size() ;
   }

   sdbLockItem* _pmdEDUCB::getLockItem( SDB_LOCK_TYPE lockType )
   {
      SDB_ASSERT( lockType >= SDB_LOCK_DMS && lockType < SDB_LOCK_MAX,
                  "lockType error" ) ;
      return &_lockInfo[ (INT32)lockType ] ;
   }

   void _pmdEDUCB::assertLocks()
   {
      for ( INT32 i = 0 ; i < SDB_LOCK_MAX ; ++i )
      {
         SDB_ASSERT( 0 == _lockInfo[i].lockCount(),
                     "Lock count must be 0" ) ;
      }

#if defined ( SDB_ENGINE )
      _transExecutor.assertLocks() ;
#endif //SDB_ENGINE
   }

   void _pmdEDUCB::resetLocks()
   {
      for ( INT32 i = 0 ; i < SDB_LOCK_MAX ; ++i )
      {
         _lockInfo[i].reset() ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB___PMDEDUCB_DUMPINFO, "_pmdEDUCB::dumpInfo" )
   void _pmdEDUCB::dumpInfo ( monEDUSimple &simple )
   {
      PD_TRACE_ENTRY ( SDB___PMDEDUCB_DUMPINFO );
      ossScopedLock _lock ( &_mutex, SHARED ) ;
      simple._eduStatus[0] = 0 ;
      simple._eduType[0] = 0 ;
      simple._eduName[0] = 0 ;
      simple._source[0] = 0 ;
      simple._eduID = _eduID ;
      simple._tid = _tid ;
      ossStrncpy ( simple._eduStatus, getEDUStatusDesp(_status),
                   MON_EDU_STATUS_SZ ) ;
      ossStrncpy ( simple._eduType, getEDUName (_eduType), MON_EDU_TYPE_SZ ) ;
      simple._eduType[ MON_EDU_TYPE_SZ ] = 0 ;
      ossStrncpy ( simple._eduName, _name, MON_EDU_NAME_SZ ) ;
      simple._eduName[ MON_EDU_NAME_SZ ] = 0 ;
      ossStrncpy ( simple._source, _source, MON_EDU_NAME_SZ ) ;
      simple._source[ MON_EDU_NAME_SZ ] = 0 ;

      if ( _pSession )
      {
         simple._relatedNID = _pSession->identifyID() ;
         simple._relatedTID = _pSession->identifyTID() ;
      }
      else
      {
         simple._relatedTID = _tid ;
         simple._relatedNID = 0 ;
      }

      PD_TRACE_EXIT ( SDB___PMDEDUCB_DUMPINFO );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB___PMDEDUCB_DUMPINFO2, "_pmdEDUCB::dumpInfo" )
   void _pmdEDUCB::dumpInfo ( monEDUFull &full )
   {
      PD_TRACE_ENTRY ( SDB___PMDEDUCB_DUMPINFO2 );
      ossScopedLock _lock ( &_mutex, SHARED ) ;
      full._eduStatus[0] = 0 ;
      full._eduType[0] = 0 ;
      full._eduName[0] = 0 ;
      full._source[0] = 0 ;
      full._eduID = _eduID ;
      full._tid = _tid ;
      full._processEventCount = _processEventCount ;
      full._queueSize = _queue.size() ;
      ossStrncpy ( full._eduStatus, getEDUStatusDesp(_status),
                   MON_EDU_STATUS_SZ ) ;
      full._eduStatus[ MON_EDU_STATUS_SZ ] = 0 ;
      ossStrncpy ( full._eduType, getEDUName (_eduType), MON_EDU_TYPE_SZ ) ;
      full._eduType[ MON_EDU_TYPE_SZ ] = 0 ;
      ossStrncpy ( full._eduName, _name, MON_EDU_NAME_SZ ) ;
      full._eduName[ MON_EDU_NAME_SZ ] = 0 ;
      ossStrncpy ( full._source, _source, MON_EDU_NAME_SZ ) ;
      full._source[ MON_EDU_NAME_SZ ] = 0 ;

      full._monApplCB = _monApplCB ;
      full._threadHdl = _threadHdl ;
      full._eduContextList = _contextList ;
      if ( _pSession )
      {
         full._relatedNID = _pSession->identifyID() ;
         full._relatedTID = _pSession->identifyTID() ;
      }
      else
      {
         full._relatedTID = _tid ;
         full._relatedNID = 0 ;
      }

      PD_TRACE_EXIT ( SDB___PMDEDUCB_DUMPINFO2 );
   }

#if defined ( SDB_ENGINE )
   void _pmdEDUCB::clearTransInfo()
   {
      _curTransID = DPS_INVALID_TRANS_ID ;
      _relatedTransLSN = DPS_INVALID_LSN_OFFSET ;
      _curTransLSN = DPS_INVALID_LSN_OFFSET ;
      _transRC = SDB_OK ;
      dpsTransCB *pTransCB = pmdGetKRCB()->getTransCB() ;
      if ( pTransCB )
      {
         pTransCB->transLockReleaseAll( this, NULL ) ;
      }
   }

   BOOLEAN _pmdEDUCB::isTransaction() const
   {
      return ( DPS_INVALID_TRANS_ID != _curTransID ) ? TRUE : FALSE ;
   }

   BOOLEAN _pmdEDUCB::isAutoCommitTrans() const
   {
      return DPS_TRANS_IS_AUTOCOMMIT( _curTransID ) ? TRUE : FALSE ;
   }

   void _pmdEDUCB::dumpTransInfo( monTransInfo &transInfo )
   {
      transInfo._eduID        = _eduID ;
      transInfo._transID      = _curTransID ;
      transInfo._curTransLsn  = _curTransLSN ;

      transInfo._lastLRB   = _transExecutor.getLastLRB() ;
      transInfo._waitLRB   = _transExecutor.getWaiterLRB() ;

      {
         ossScopedLock lock( &_mutex, SHARED ) ;
         if ( _pSession )
         {
            transInfo._relatedNID = _pSession->identifyID() ;
            transInfo._relatedTID = _pSession->identifyTID() ;
         }
         else
         {
            transInfo._relatedTID = _tid ;
            transInfo._relatedNID = 0 ;
         }
      }
   }

   pmdTransExecutor* _pmdEDUCB::getTransExecutor()
   {
      return &_transExecutor ;
   }

#endif // SDB_ENGINE

   static OSS_THREAD_LOCAL _pmdEDUCB *__eduCB ;

   _pmdEDUCB *pmdGetThreadEDUCB ()
   {
      return __eduCB ;
   }

   _pmdEDUCB *pmdDeclareEDUCB ( _pmdEDUCB *p )
   {
      __eduCB = p ;
      return __eduCB ;
   }

   void pmdUndeclareEDUCB ()
   {
      __eduCB = NULL ;
   }

   // for pmdRecv, we wait indefinitely until the agent is forced, because
   // client may not send us anything due to idle of user activities
   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDRECV, "pmdRecv" )
   INT32 pmdRecv ( CHAR *pBuffer, INT32 recvSize,
                   ossSocket *sock, pmdEDUCB *cb,
                   INT32 timeout,
                   INT32 forceTimeout )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( sock, "Socket is NULL" ) ;
      SDB_ASSERT ( cb, "cb is NULL" ) ;
      PD_TRACE_ENTRY ( SDB_PMDRECV );
      INT32 receivedSize = 0 ;
      INT32 totalReceivedSize = 0 ;
      while ( TRUE )
      {
         if ( cb->isForced () )
         {
            rc = SDB_APP_FORCED ;
            goto done ;
         }
         rc = sock->recv ( &pBuffer[totalReceivedSize],
                           recvSize-totalReceivedSize,
                           receivedSize, timeout ) ;
         totalReceivedSize += receivedSize ;
         if ( SDB_TIMEOUT == rc )
         {
            if ( forceTimeout > 0 && timeout > 0 )
            {
               if ( forceTimeout > timeout )
               {
                  forceTimeout -= timeout ;
               }
               else
               {
                  break ;
               }
            }
            continue ;
         }
         goto done ;
      }
   done :
#if defined ( SDB_ENGINE )
      if ( totalReceivedSize > 0 )
      {
         pmdGetKRCB()->getMonDBCB()->svcNetInAdd( totalReceivedSize ) ;
      }
#endif // SDB_ENGINE
      PD_TRACE_EXITRC ( SDB_PMDRECV, rc );
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_PMDSEND, "pmdSend" )
   INT32 pmdSend ( const CHAR *pBuffer, INT32 sendSize,
                   ossSocket *sock, pmdEDUCB *cb,
                   INT32 timeout )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( sock, "Socket is NULL" ) ;
      SDB_ASSERT ( cb, "cb is NULL" ) ;
      PD_TRACE_ENTRY ( SDB_PMDSEND );
      INT32 sentSize = 0 ;
      INT32 totalSentSize = 0 ;
      while ( true )
      {
         if ( cb->isForced () )
         {
            rc = SDB_APP_FORCED ;
            goto done ;
         }
         rc = sock->send ( &pBuffer[totalSentSize],
                           sendSize-totalSentSize,
                           sentSize, timeout ) ;
         totalSentSize += sentSize ;
         if ( SDB_TIMEOUT == rc )
            continue ;
         goto done ;
      }
   done :
#if defined ( SDB_ENGINE )
      if ( totalSentSize > 0 )
      {
         pmdGetKRCB()->getMonDBCB()->svcNetOutAdd( totalSentSize ) ;
      }
#endif // SDB_ENGINE
      PD_TRACE_EXITRC ( SDB_PMDSEND, rc );
      return rc ;
   }

   INT32 pmdSyncSendMsg( const MsgHeader *pMsg, MsgHeader **ppRecvMsg,
                         ossSocket *sock, pmdEDUCB *cb, BOOLEAN useCBMem,
                         INT32 timeout, INT32 forceTimeout )
   {
      INT32 rc = SDB_OK ;
      UINT32 msgLen = 0 ;
      CHAR *pRecvBuf = NULL ;
      rc = pmdSend( (const CHAR *)pMsg, pMsg->messageLength, sock,
                    cb, timeout ) ;
      if ( rc )
      {
         goto error ;
      }
      // recieve msg length
      rc = pmdRecv( (CHAR*)&msgLen, sizeof(INT32), sock, cb, timeout,
                    forceTimeout ) ;
      if ( rc )
      {
         goto error ;
      }
      if ( msgLen < sizeof( MsgHeader ) || msgLen > SDB_MAX_MSG_LENGTH )
      {
         PD_LOG( PDERROR, "Recieve msg size[%u] less than msg header or more "
                 "than max size", msgLen ) ;
         rc = SDB_INVALIDARG ;
         sock->close() ;
         goto error ;
      }
      // alloc memory
      if ( useCBMem )
      {
         rc = cb->allocBuff( msgLen, &pRecvBuf, NULL ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      else
      {
         pRecvBuf = ( CHAR* )SDB_OSS_MALLOC( msgLen ) ;
         if ( !pRecvBuf )
         {
            PD_LOG( PDERROR, "Alloc memory failed, size: %u", msgLen ) ;
            rc = SDB_OOM ;
            goto error ;
         }
      }
      ossMemcpy( pRecvBuf, ( CHAR* )&msgLen, sizeof( INT32 ) ) ;
      // recieve last msg
      rc = pmdRecv( pRecvBuf + sizeof( INT32 ), msgLen - sizeof( INT32 ),
                    sock, cb, timeout ) ;
      if ( rc )
      {
         goto error ;
      }
      *ppRecvMsg = ( MsgHeader* )pRecvBuf ;

   done:
      return rc ;
   error:
      if ( pRecvBuf )
      {
         if ( useCBMem )
         {
            cb->releaseBuff( pRecvBuf ) ;
         }
         else
         {
            SDB_OSS_FREE( pRecvBuf ) ;
         }
         pRecvBuf = NULL ;
      }
      goto done ;
   }

   INT32 pmdSendAndRecv2Que( const MsgHeader *pMsg, ossSocket *sock,
                             pmdEDUCB *cb, INT32 timeout,
                             INT32 forceTimeout )
   {
      INT32 rc = SDB_OK ;
      MsgHeader *pRecvMsg = NULL ;
      pmdEDUEvent event ;
      rc = pmdSyncSendMsg( pMsg, &pRecvMsg, sock, cb, FALSE, timeout,
                           forceTimeout ) ;
      if ( rc )
      {
         goto error ;
      }
      event._Data = (void*)pRecvMsg ;
      event._dataMemType = PMD_EDU_MEM_ALLOC ;
      event._eventType = PMD_EDU_EVENT_MSG ;
      event._userData = 0 ;
      cb->postEvent( event ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void pmdEduEventRelase( pmdEDUEvent &event, pmdEDUCB *cb )
   {
      if ( event._Data && event._dataMemType != PMD_EDU_MEM_NONE )
      {
         if ( PMD_EDU_MEM_ALLOC == event._dataMemType )
         {
            SDB_OSS_FREE( event._Data ) ;
         }
         else if ( PMD_EDU_MEM_SELF == event._dataMemType )
         {
            SDB_ASSERT( cb, "cb can't be NULL" ) ;
            if ( cb )
            {
               cb->releaseBuff( (CHAR *)event._Data ) ;
            }
         }
         event._Data = NULL ;
      }
   }

}

