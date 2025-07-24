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

   Source File Name = pmdEDU.hpp

   Descriptive Name = Process MoDel Engine Dispatchable Unit Header

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains structure for EDU Control
   Block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMDEDU_HPP__
#define PMDEDU_HPP__

#include "sdbInterface.hpp"
#include "ossLatch.hpp"
#include "ossQueue.hpp"
#include "ossMem.hpp"
#include "ossSocket.hpp"
#include "oss.hpp"
#include "pmdDef.hpp"
#include "ossRWMutex.hpp"
#include "sdbInterface.hpp"
#include "pdTrace.hpp"
#include "dpsDef.hpp"

#if defined ( SDB_ENGINE )
#include "monEDU.hpp"
#include "monCB.hpp"
#include "dpsLogDef.hpp"
#include "dpsTransCB.hpp"
#include "dpsTransLockDef.hpp"
#endif // SDB_ENGINE

#include <set>
#include <string>

using namespace std ;

namespace engine
{
   /*
      CONST VALUE DEFINE
   */
   #define PMD_EDU_NAME_LENGTH         ( 512 )
   #define EDU_ERROR_BUFF_SIZE         ( 1024 )

   /*
      TOOL FUNCTIONS
   */
   INT32       registerEDUName ( EDU_TYPES type, const CHAR * name,
                                 BOOLEAN system ) ;
   const CHAR *getEDUStatusDesp ( EDU_STATUS status ) ;
   const CHAR *getEDUName ( EDU_TYPES type ) ;
   BOOLEAN     isSystemEDU ( EDU_TYPES type ) ;

   /*
      EDU CONTROL FLAG DEFINE
   */
   #define EDU_CTRL_INTERRUPTED        0x01
   #define EDU_CTRL_DISCONNECTED       0x02
   #define EDU_CTRL_FORCED             0x04

   /*
      EDU INFO TYPE DEFINE
   */
   enum EDU_INFO_TYPE
   {
      EDU_INFO_ERROR                   = 1   //Error
   } ;

   class _pmdEDUMgr ;
   class CoordSession ;

   /*
      _pmdEDUCB define
   */
   class _pmdEDUCB : public _IExecutor
   {
   public:
      typedef std::multimap<UINT32,CHAR*>    CATCH_MAP ;
      typedef CATCH_MAP::iterator            CATCH_MAP_IT ;

      typedef std::map<CHAR*,UINT32>         ALLOC_MAP ;
      typedef ALLOC_MAP::iterator            ALLOC_MAP_IT ;

      friend class _pmdEDUMgr ;

   public:
         /*
            Base Function
         */
         virtual EDUID     getID() const { return _eduID ; }
         virtual UINT32    getTID() const { return _tid ; }

         /*
            Session Related
         */
         virtual ISession* getSession() { return _pSession ; }

         /*
            Status and Control
         */
         virtual BOOLEAN   isInterrupted ( BOOLEAN onlyFlag = FALSE ) ;
         virtual BOOLEAN   isDisconnected () ;
         virtual BOOLEAN   isForced () ;

         virtual BOOLEAN   isWritingDB() const { return _writingDB ; }
         virtual UINT64    getWritingID() const { return _writingID ; }
         virtual void      writingDB( BOOLEAN writing ) ;

         virtual UINT32    getProcessedNum() const { return _processEventCount ; }
         virtual void      incEventCount( UINT32 step = 1 ) ;

         virtual UINT32    getQueSize() ;

         /*
            Resource Info
         */
         virtual sdbLockItem* getLockItem( SDB_LOCK_TYPE lockType ) ;
         void                 assertLocks() ;
         void                 resetLocks() ;

         /*
            Buffer Manager
         */
         virtual INT32     allocBuff( UINT32 len,
                                      CHAR **ppBuff,
                                      UINT32 *pRealSize = NULL ) ;

         virtual INT32     reallocBuff( UINT32 len,
                                        CHAR **ppBuff,
                                        UINT32 *pRealSize = NULL ) ;

         virtual void      releaseBuff( CHAR *pBuff ) ;

         virtual void*     getAlignedBuff( UINT32 size,
                                           UINT32 *pRealSize = NULL,
                                           UINT32 alignment =
                                           OSS_FILE_DIRECT_IO_ALIGNMENT ) ;

         virtual void      releaseAlignedBuff() ;

         /*
            Operation Related
         */
         virtual UINT64    getBeginLsn () const { return _beginLsn ; }
         virtual UINT64    getEndLsn() const { return _endLsn ; }
         virtual UINT32    getLsnCount () const { return _lsnNumber ; }
         virtual BOOLEAN   isDoRollback () const { return _doRollback ; }
         virtual UINT64    getTransID () const { return _curTransID ; }
         virtual UINT64    getCurTransLsn () const { return _curTransLSN ; }

         virtual void      resetLsn() ;
         virtual void      insertLsn( UINT64 lsn,
                                      BOOLEAN isRollback = FALSE ) ;
         virtual void      setTransID( UINT64 transID ) ;
         virtual void      setCurTransLsn( UINT64 lsn ) ;

   public:
      _pmdEDUCB( _pmdEDUMgr *mgr, EDU_TYPES type ) ;
      ~_pmdEDUCB() ;

      void        clear() ;
      string      toString() const ;

      EDU_STATUS  getStatus () const { return _status ; }
      EDU_TYPES   getType () const { return _eduType ; }

      _pmdEDUMgr* getEDUMgr() { return _eduMgr ; }

      void        setTID ( UINT32 tid ) { _tid = tid ; }
      void        setType ( EDU_TYPES type ) ;

   #if defined ( _LINUX )
      void        setThreadID ( pthread_t id ) { _threadID = id ; }
      pthread_t   getThreadID () const { return _threadID ; }
      void        setThreadHdl( OSSTID hdl ) { _threadHdl = hdl ; }
      OSSTID      getThreadHandle() const { return _threadHdl ; }
   #elif defined ( _WINDOWS )
      void        setThreadHdl( HANDLE hdl ) { _threadHdl = hdl ; }
      HANDLE      getThreadHandle() const { return _threadHdl ; }
   #endif

   public :
      void        attachSession( ISession *pSession ) ;
      void        detachSession() ;

      void        restoreBuffs( CATCH_MAP &catchMap ) ;
      void        saveBuffs( CATCH_MAP &catchMap ) ;

      CHAR*       getCompressBuff( UINT32 len ) ;
      INT32       getCompressBuffLen() const { return _compressBuffLen ; }
      CHAR*       getUncompressBuff( UINT32 len ) ;
      INT32       getUncompressBuffLen() const { return _uncompressBuffLen ; }

      void        interrupt () ;
      void        resetInterrupt () ;
      void        resetDisconnect () ;

      INT32       printInfo ( EDU_INFO_TYPE type, const CHAR *format, ... ) ;
      const CHAR* getInfo ( EDU_INFO_TYPE type ) ;
      void        resetInfo ( EDU_INFO_TYPE type ) ;

      void        setUserInfo( const string &userName,
                               const string &password ) ;
      void        setName ( const CHAR *name ) ;
      void        setClientSock ( ossSocket *pSock ) { _pClientSock = pSock ; }
      ossSocket*  getClientSock () { return _pClientSock ; }

      BOOLEAN     isFromLocal() const { return _pClientSock ? TRUE : FALSE ; }
      const CHAR* getName () ;
      const CHAR* getUserName() const { return _userName.c_str() ; }
      const CHAR* getPassword() const { return _passWord.c_str() ; }

      void postEvent ( pmdEDUEvent const &data )
      {
         // no need latch since _queue is already latched
         _queue.push ( data ) ;
      }

      BOOLEAN waitEvent ( pmdEDUEvent &data, INT64 millsec,
                          BOOLEAN resetStat = FALSE )
      {
         // no need latch since _queue is already latched
         // if millsec not 0, that means we want timeout
         // otherwise it's infinite wait

         BOOLEAN waitMsg   = FALSE ;
         writingDB( FALSE ) ;

         if ( resetStat && PMD_EDU_IDLE != _status )
         {
            _status = PMD_EDU_WAITING ;
         }

         if ( 0 > millsec )
         {
            _queue.wait_and_pop ( data ) ;
            waitMsg = TRUE ;
         }
         else
         {
            waitMsg = _queue.timed_wait_and_pop ( data, millsec ) ;
         }

         if ( waitMsg )
         {
            ++_processEventCount ;
            if ( data._eventType == PMD_EDU_EVENT_TERM )
            {
               _ctrlFlag |= ( EDU_CTRL_DISCONNECTED|EDU_CTRL_INTERRUPTED );
            }
            else if ( resetStat )
            {
               _status = PMD_EDU_RUNNING ;
            }
         }

         return waitMsg ;
      }

      BOOLEAN waitEvent( pmdEDUEventTypes type, pmdEDUEvent &data,
                         INT64 millsec, BOOLEAN resetStat = FALSE )
      {
         BOOLEAN ret = FALSE ;
         INT64 waitTime = 0 ;
         ossQueue< pmdEDUEvent > tmpQue ;

         if ( millsec < 0 )
         {
            millsec = 0x7FFFFFFF ;
         }

         while ( !isInterrupted() )
         {
            waitTime = millsec < OSS_ONE_SEC ? millsec : OSS_ONE_SEC ;
            if ( !waitEvent( data, waitTime, resetStat ) )
            {
               millsec -= waitTime ;
               if ( millsec <= 0 )
               {
                  break ;
               }
               continue ;
            }
            if ( type != data._eventType )
            {
               tmpQue.push( data ) ;
               --_processEventCount ;
               continue ;
            }
            ret = TRUE ;
            break ;
         }

         pmdEDUEvent tmpData ;
         while ( !tmpQue.empty() )
         {
            tmpQue.try_pop( tmpData ) ;
            _queue.push( tmpData ) ;
         }
         return ret ;
      }

   #if defined ( SDB_ENGINE )

      void  initMonAppCB()
      {
         _monApplCB.reset() ;
         if ( _monCfgCB.timestampON )
         {
            _monApplCB.recordConnectTimestamp() ;
         }
      }
      void resetMon () { _monApplCB.reset () ; }
      monConfigCB * getMonConfigCB() { return & _monCfgCB ; }
      monAppCB * getMonAppCB() { return & _monApplCB ; }

      ossEvent & getEvent () { return _event ; }

      void setCoordSession( CoordSession *pSession )
      {
         _pCoordSession = pSession;
      }
      CoordSession *getCoordSession() { return _pCoordSession ; }

      void contextInsert ( SINT64 contextID )
      {
         ossScopedLock _lock ( &_mutex, EXCLUSIVE ) ;
         _contextList.insert ( contextID ) ;
      }
      void contextDelete ( SINT64 contextID )
      {
         ossScopedLock _lock ( &_mutex, EXCLUSIVE ) ;
         _contextList.erase ( contextID ) ;
      }
      SINT64 contextPeek () ;
      BOOLEAN contextFind ( SINT64 contextID )
      {
         ossScopedLock _lock ( &_mutex, SHARED ) ;
         return _contextList.end() != _contextList.find( contextID ) ;
      }
      UINT32 contextNum ()
      {
         ossScopedLock _lock ( &_mutex, SHARED ) ;
         return _contextList.size() ;
      }
      void contextCopy ( std::set<SINT64> &contextList )
      {
         ossScopedLock _lock ( &_mutex, SHARED ) ;
         contextList = _contextList ;
      }

      UINT64 getCurRequestID() const { return _curRequestID ; }
      // WANRING: no lock protect, only called by eduCB thread itself
      UINT64 incCurRequestID() { return ++_curRequestID ; }

      // transaction related
      void     setRelatedTransLSN( DPS_LSN_OFFSET relatedLSN )
      {
         _relatedTransLSN = relatedLSN ;
      }
      DPS_LSN_OFFSET getRelatedTransLSN() const { return _relatedTransLSN ; }
      dpsTransCBLockInfo *getTransLock( const dpsTransLockId &lockId );
      void     addLockInfo( const dpsTransLockId &lockId,
                            DPS_TRANSLOCK_TYPE lockType ) ;
      void     delLockInfo( const dpsTransLockId &lockId ) ;
      DpsTransCBLockList *getLockList() ;
      void     clearLockList() ;
      INT32    createTransaction() ;
      void     delTransaction() ;
      void     addTransNode( MsgRouteID &routeID ) ;
      void     delTransNode( MsgRouteID &routeID ) ;
      void     getTransNodeRouteID( UINT32 groupID, MsgRouteID &routeID ) ;
      DpsTransNodeMap *getTransNodeLst() ;
      BOOLEAN  isTransaction() ;
      BOOLEAN  isTransNode( MsgRouteID &routeID ) ;
      void     startRollback() { _isDoRollback = TRUE ; }
      void     stopRollback() { _isDoRollback = FALSE ; }
      BOOLEAN  isInRollback() const { return _isDoRollback ; }
      void     setTransRC( INT32 rc ) { _transRC = rc ; }
      INT32    getTransRC() const { return _transRC ; }
      void     clearTransInfo() ;
      void     setWaitLock( const dpsTransLockId &lockId ) ;
      void     clearWaitLock() ;

      void     dumpInfo ( monEDUSimple &simple ) ;
      void     dumpInfo ( monEDUFull &full ) ;

      void     dumpTransInfo( monTransInfo &transInfo ) ;
      void     setOrgReplSize( INT16 replSize ) { _orgReplSize = replSize ; }
      INT16    getOrgReplSize() const { return _orgReplSize ; }

   #endif // SDB_ENGINE

   protected:
      void     setStatus ( EDU_STATUS status ) { _status = status ; }
      void     setID ( EDUID id ) { _eduID = id ; }

      CHAR*    _getBuffInfo ( EDU_INFO_TYPE type, UINT32 &size ) ;
      BOOLEAN  _allocFromCatch( UINT32 len, CHAR **ppBuff, UINT32 *buffLen ) ;

      void     disconnect () ;
      void     force () ;

   private :
      _pmdEDUMgr     *_eduMgr ;
      ossSpinSLatch  _mutex ;
      ossQueue<pmdEDUEvent> _queue ;

      EDU_STATUS     _status ;
      EDU_TYPES      _eduType ;

      string         _userName ;
      string         _passWord ;

      // buffer related
      CHAR           *_pCompressBuff ;
      UINT32         _compressBuffLen ;
      CHAR           *_pUncompressBuff ;
      UINT32         _uncompressBuffLen ;

      CATCH_MAP      _catchMap ;
      ALLOC_MAP      _allocMap ;
      INT64          _totalCatchSize ;
      INT64          _totalMemSize ;

      // thread specific error message buffer, aka SQLCA
      CHAR              *_pErrorBuff ;
   #if defined ( _WINDOWS )
      HANDLE            _threadHdl ;
   #elif defined ( _LINUX )
      OSSTID            _threadHdl ;
      pthread_t         _threadID ;
   #endif // _WINDOWS

      CHAR              _name[ PMD_EDU_NAME_LENGTH + 1 ] ;
      ossSocket        *_pClientSock ;

      BOOLEAN                 _isDoRollback ;

   #if defined ( SDB_ENGINE )

      monAppCB                _monApplCB ;
      monConfigCB             _monCfgCB ;

      ossEvent                _event ;   // for cls replSet notify

      std::set<SINT64>        _contextList ;

      // coord related variables
      CoordSession            *_pCoordSession;

      UINT64                  _curRequestID ;

      // transaction related variables
      DPS_LSN_OFFSET          _relatedTransLSN ;
      ossSpinXLatch           _transLockLstMutex ;
      DpsTransCBLockList      _transLockLst ;
      DpsTransNodeMap         *_pTransNodeMap ;
      INT32                   _transRC ;
      dpsTransLockId          _waitLock ;

      INT16                   _orgReplSize ;
   #endif // SDB_ENGINE

      /*
         Interace members
      */
      EDUID                   _eduID ;
      UINT32                  _tid ;
      ISession                *_pSession ;

      UINT64                  _beginLsn ;
      UINT64                  _endLsn ;
      UINT32                  _lsnNumber ;
      BOOLEAN                 _doRollback ;
      UINT64                  _processEventCount ;

      DPS_TRANS_ID            _curTransID ;
      DPS_LSN_OFFSET          _curTransLSN ;

      sdbLockItem             _lockInfo[ SDB_LOCK_MAX ] ;

      INT32                   _ctrlFlag ;
      BOOLEAN                 _writingDB ;
      UINT64                  _writingID ;
      /// aligned memory.
      void                    *_alignedMem ;
      UINT32                   _alignedMemSize ;

   };
   typedef class _pmdEDUCB pmdEDUCB ;

   _pmdEDUCB *pmdGetThreadEDUCB () ;
   _pmdEDUCB *pmdDeclareEDUCB ( _pmdEDUCB *p ) ;
   void      pmdUndeclareEDUCB () ;

   /*
      ENTRY POINT
   */
   typedef INT32 (*pmdEntryPoint)( pmdEDUCB *, void * ) ;

   pmdEntryPoint getEntryFuncByType ( EDU_TYPES type ) ;

   INT32 pmdEDUEntryPoint ( EDU_TYPES type, pmdEDUCB *cb, void *arg ) ;
   INT32 pmdEDUEntryPointWrapper ( EDU_TYPES type, pmdEDUCB *cb, void *arg ) ;

   // array assignment later, can't inherit from SDBObject
   struct _eduEntryInfo
   {
      EDU_TYPES         type ;
      INT32             regResult ;
      pmdEntryPoint     entryFunc ;
   } ;

#define ON_EDUTYPE_TO_ENTRY1(type, system, entry, desp) \
   { type, registerEDUName(type, desp, system), entry }

#define ON_EDUTYPE_TO_ENTRY2(type, system, entry) \
   ON_EDUTYPE_TO_ENTRY1(type, system, entry, #type)

   /*
      TOOL FUNCTIONS
   */
   INT32 pmdRecv ( CHAR *pBuffer, INT32 recvSize,
                   ossSocket *sock, pmdEDUCB *cb,
                   INT32 timeout = OSS_SOCKET_DFT_TIMEOUT,
                   INT32 forceTimeout = -1 ) ;
   INT32 pmdSend ( const CHAR *pBuffer, INT32 sendSize,
                   ossSocket *sock, pmdEDUCB *cb,
                   INT32 timeout = OSS_SOCKET_DFT_TIMEOUT ) ;
   /*
      NOTE: the ppRecvMsg is alloced, so need to free
      useCBMem: TRUE: the memory is allocated by cb->allocBuf, so must free
                by cb->releaseBuf
                FALSE: the memory is allocated by SDB_OSS_MALLOC, so must
                free by SDB_OSS_FREE
   */
   INT32 pmdSyncSendMsg( const MsgHeader *pMsg, MsgHeader **ppRecvMsg,
                         ossSocket *sock, pmdEDUCB *cb,
                         BOOLEAN useCBMem = TRUE,
                         INT32 timeout = OSS_SOCKET_DFT_TIMEOUT,
                         INT32 forceTimeout = -1 ) ;

   /*
      NOTE: recv the msg to cb queue
   */
   INT32 pmdSendAndRecv2Que( const MsgHeader *pMsg, ossSocket *sock,
                             pmdEDUCB *cb,
                             INT32 timeout = OSS_SOCKET_DFT_TIMEOUT,
                             INT32 forceTimeout = -1 ) ;

   void  pmdEduEventRelase( pmdEDUEvent &event, pmdEDUCB *cb ) ;

}

#endif // PMDEDU_HPP__

