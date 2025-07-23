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

   Source File Name = dpsTransExecutor.hpp

   Descriptive Name = Operating System Services Types Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/08/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPS_TRANS_EXECUTOR_HPP__
#define DPS_TRANS_EXECUTOR_HPP__

#include "sdbInterface.hpp"
#include "dpsTransLockDef.hpp"
#include "dpsTransDef.hpp"
#include "dpsTransLockMgr.hpp"
#include "utilSegment.hpp"
#include "ossMemPool.hpp"
#include "dpsDef.hpp"

using namespace bson ;
using namespace std ;

namespace engine
{

   class dpsTransLRBHeader;

   /*
      DPS_TRANS_QUE_TYPE define
   */
   enum DPS_TRANS_QUE_TYPE
   {
      DPS_QUE_NULL         = 0,
      DPS_QUE_UPGRADE,
      DPS_QUE_WAITER
   } ;

   /*
      _dpsTransConfItem define
   */
   class _dpsTransConfItem
   {
      public:
         _dpsTransConfItem() ;
         virtual ~_dpsTransConfItem() ;

      public:
         INT32                getTransIsolation() const ;
         UINT32               getTransTimeout() const ;
         BOOLEAN              isTransWaitLock() const ;
         BOOLEAN              useRollbackSegment() const ;
         BOOLEAN              isTransAutoCommit() const ;
         BOOLEAN              isTransAutoRollback() const ;

         UINT32               getTransConfMask() const ;
         UINT32               getTransConfVer() const ;

         void                 setTransIsolation( INT32 isolation,
                                                 BOOLEAN enableMask = TRUE ) ;
         void                 setTransTimeout( UINT32 timeout,
                                               BOOLEAN enableMask = TRUE ) ;
         void                 setTransWaitLock( BOOLEAN waitLock,
                                                BOOLEAN enableMask = TRUE ) ;
         void                 setUseRollbackSemgent( BOOLEAN use,
                                                     BOOLEAN enableMask = TRUE ) ;
         void                 setTransAutoCommit( BOOLEAN autoCommit,
                                                  BOOLEAN enableMask = TRUE ) ;
         void                 setTransAutoRollback( BOOLEAN autoRollback,
                                                    BOOLEAN enableMask = TRUE ) ;

         void                 reset() ;
         void                 resetConfMask() ;
         void                 resetConfMask( UINT32 bitMask ) ;

         void                 updateByMask( const _dpsTransConfItem &rhs ) ;
         void                 copyFrom( const _dpsTransConfItem &rhs ) ;

         void                 toBson( BSONObjBuilder &builder ) const ;
         void                 fromBson( const BSONObj &obj ) ;

      protected:
         INT32                   _transIsolation ;
         UINT32                  _transTimeout ;      /// Unit:ms
         // if transaction wait for lock
         BOOLEAN                 _transWaitLock ;  
         // if transaction use old copy in rollback segment
         BOOLEAN                 _useRollbackSegment ;
         // insert/update/delete/query operator auto use transaction
         BOOLEAN                 _transAutoCommit ;
         // when transaction operator failed, wether rollback auto
         BOOLEAN                 _transAutoRollback ;

         UINT32                  _transConfMask ;
         UINT32                  _transConfVer ;

   } ;
   typedef _dpsTransConfItem dpsTransConfItem ;

   /*
      _dpsTransExecutor define
   */
   class _dpsTransExecutor : public _dpsTransConfItem
   {
      struct cmpCSCLLock
      {
         bool operator() ( const dpsTransLockId& lhs, 
                           const dpsTransLockId& rhs ) const 
         {
            if ( lhs.csID() < rhs.csID() )
            {
               return TRUE ;
            }
            else if ( lhs.csID() > rhs.csID() )
            {
               return FALSE ;
            }
            if ( lhs.clID() < rhs.clID() )
            {
               return TRUE ;
            }
            else if ( lhs.clID() > rhs.clID() )
            {
               return FALSE ;
            }
            return FALSE ;
         }
      };

      // Only CS and CL lock should be inserted in this map. If other locks
      // are to be inserted, the cmpCSCLLock compare function needs to be updated
      typedef ossPoolMap< dpsTransLockId,
                          dpsTransLRB*, 
                          cmpCSCLLock >                  DPS_LOCKID_MAP ;
      typedef DPS_LOCKID_MAP::iterator                   DPS_LOCKID_MAP_IT ;
      typedef DPS_LOCKID_MAP::const_iterator             DPS_LOCKID_MAP_CIT ;

      typedef ossPoolMap<DPS_LSN_OFFSET,dmsRecordID>     MAP_LSN_2_RECORD ;
      typedef MAP_LSN_2_RECORD::iterator                 MAP_LSN_2_RECORD_IT ;
      typedef MAP_LSN_2_RECORD::const_iterator           MAP_LSN_2_RECORD_CIT ;

      friend class _pmdEDUCB ;

      public:
         _dpsTransExecutor() ;
         virtual ~_dpsTransExecutor() ;

         void     clearAll() ;
         void     assertLocks() ;

      public:

         void                 setWaiterInfo( dpsTransLRB * lrb,
                                             DPS_TRANS_QUE_TYPE type ) ;
         void                 clearWaiterInfo() ;

         dpsTransLRB*         getWaiterLRB() const ;
         DPS_TRANS_QUE_TYPE   getWaiterQueType() const ;

         void                 setLastLRB( dpsTransLRB *lrb ) ;
         void                 clearLastLRB() ;
         dpsTransLRB *          getLastLRB() const ;

         BOOLEAN              addLock( const dpsTransLockId &lockID,
                                       dpsTransLRB * lrb ) ;
         BOOLEAN              findLock( const dpsTransLockId &lockID,
                                        dpsTransLRB * &lrb ) const ;
         BOOLEAN              removeLock( const dpsTransLockId &lockID ) ;
         void                 clearLock() ;

         void                 incLockCount() ;
         void                 decLockCount() ;
         void                 clearLockCount() ;
         UINT32               getLockCount() const ;

         /*
            Transaction Related
         */
         void                 setUseTransLock( BOOLEAN use ) ;
         BOOLEAN              useTransLock() const ;

         /*
            LSN to record map functions
         */
         const MAP_LSN_2_RECORD*    getRecordMap() const ;
         void                       putRecord( DPS_LSN_OFFSET lsnOffset,
                                               const dmsRecordID &item ) ;
         void                       delRecord( DPS_LSN_OFFSET lsnOffset ) ;
         BOOLEAN                    getRecord( DPS_LSN_OFFSET lsnOffset,
                                               dmsRecordID &item,
                                               BOOLEAN withDel = FALSE ) ;
         void                       clearRecordMap() ;
         BOOLEAN                    isRecordMapEmpty() const ;
         UINT32                     getRecordMapSize() const ;

      protected:
         void                 initTransConf( INT32 isolation,
                                             UINT32 timeout,
                                             BOOLEAN waitLock,
                                             BOOLEAN autoCommit,
                                             BOOLEAN autoRollback,
                                             BOOLEAN useRBS ) ;

         BOOLEAN              updateTransConf( INT32 isolation,
                                               UINT32 timeout,
                                               BOOLEAN waitLock,
                                               BOOLEAN autoCommit,
                                               BOOLEAN autoRollback,
                                               BOOLEAN useRBS ) ;

         void     addReservedSpace( const UINT64 len ) ;

         UINT64   getReservedSpace() const ;

         void     resetLogSpace() ;
     
      public:
         /*
            Interface
         */
         virtual EDUID        getEDUID() const = 0 ;
         virtual UINT32       getTID() const = 0 ;
         virtual void         wakeup() = 0 ;
         virtual INT32        wait( INT64 timeout ) = 0 ;
         virtual IExecutor*   getExecutor() = 0 ;

      protected:
         dpsTransLRB *           _waiter ;
         DPS_TRANS_QUE_TYPE      _waiterQueType ;
         dpsTransLRB *           _lastLRB ;

         DPS_LOCKID_MAP          _mapCSCLLockID ;
         UINT32                  _lockCount ;

         /*
            LSN to record info
         */
         MAP_LSN_2_RECORD        _mapLSN2Record ;

      private:
         BOOLEAN                 _useTransLock ;
         // undo LR space reserved by this transaction
         UINT64                  _reservedLogSpace ;

   } ;
   typedef _dpsTransExecutor dpsTransExecutor ;

}

#endif // DPS_TRANS_EXECUTOR_HPP__

