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

   Source File Name = dpsTransExecutor.cpp

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
#include "dpsTransExecutor.hpp"
#include "dpsTransLockDef.hpp"
#include "dpsTransCB.hpp"
#include "dpsTransLRB.hpp"

using namespace bson ;

namespace engine
{

   /*
      _dpsTransConfItem implement
   */
   _dpsTransConfItem::_dpsTransConfItem()
   {
      reset() ;
   }

   _dpsTransConfItem::~_dpsTransConfItem()
   {
   }

   void _dpsTransConfItem::reset()
   {
      _transIsolation   = DPS_TRANS_ISOLATION_DFT ;
      _transTimeout     = DPS_TRANS_DFT_TIMEOUT * OSS_ONE_SEC ;
      _transWaitLock    = DPS_TRANS_LOCKWAIT_DFT ;
      _useRollbackSegment = TRUE ;
      _transAutoCommit  = FALSE ;
      _transAutoRollback= TRUE ;
      _transConfMask    = 0 ;
      _transConfVer     = 1 ;
   }

   void _dpsTransConfItem::resetConfMask()
   {
      _transConfMask = 0 ;
   }

   void _dpsTransConfItem::resetConfMask( UINT32 bitMask )
   {
      OSS_BIT_CLEAR( _transConfMask, bitMask ) ;
   }

   INT32 _dpsTransConfItem::getTransIsolation() const
   {
      return _transIsolation ;
   }

   UINT32 _dpsTransConfItem::getTransTimeout() const
   {
      return _transTimeout ;
   }

   BOOLEAN _dpsTransConfItem::isTransWaitLock() const
   {
      return _transWaitLock ;
   }

   BOOLEAN _dpsTransConfItem::useRollbackSegment() const
   {
      return _useRollbackSegment ;
   }

   BOOLEAN _dpsTransConfItem::isTransAutoCommit() const
   {
      return _transAutoCommit ;
   }

   BOOLEAN _dpsTransConfItem::isTransAutoRollback() const
   {
      return _transAutoRollback ;
   }

   UINT32 _dpsTransConfItem::getTransConfMask() const
   {
      return _transConfMask ;
   }

   UINT32 _dpsTransConfItem::getTransConfVer() const
   {
      return _transConfVer ;
   }

   void _dpsTransConfItem::setTransIsolation( INT32 isolation,
                                              BOOLEAN enableMask )
   {
      if ( isolation >= TRANS_ISOLATION_RU &&
           isolation <= TRANS_ISOLATION_MAX - 1 &&
           _transIsolation != isolation )
      {
         _transIsolation = isolation ;
         ++_transConfVer ;
      }
      if ( enableMask )
      {
         _transConfMask |= TRANS_CONF_MASK_ISOLATION ;
      }
   }

   void _dpsTransConfItem::setTransTimeout( UINT32 timeout,
                                            BOOLEAN enableMask )
   {
      if ( _transTimeout != timeout )
      {
         _transTimeout = timeout ;
         ++_transConfVer ;
      }
      if ( enableMask )
      {
         _transConfMask |= TRANS_CONF_MASK_TIMEOUT ;
      }
   }

   void _dpsTransConfItem::setTransWaitLock( BOOLEAN waitLock,
                                             BOOLEAN enableMask )
   {
      if ( _transWaitLock != waitLock )
      {
         _transWaitLock = waitLock ;
         ++_transConfVer ;
      }
      if ( enableMask )
      {
         _transConfMask |= TRANS_CONF_MASK_WAITLOCK ;
      }
   }

   void _dpsTransConfItem::setUseRollbackSemgent( BOOLEAN use,
                                                  BOOLEAN enableMask )
   {
      if ( _useRollbackSegment != use )
      {
         _useRollbackSegment = use ;
         ++_transConfVer ;
      }
      if ( enableMask )
      {
         _transConfMask |= TRANS_CONF_MASK_USERBS ;
      }
   }

   void _dpsTransConfItem::setTransAutoCommit( BOOLEAN autoCommit,
                                               BOOLEAN enableMask )
   {
      if ( _transAutoCommit != autoCommit )
      {
         _transAutoCommit = autoCommit ;
         ++_transConfVer ;
      }
      if ( enableMask )
      {
         _transConfMask |= TRANS_CONF_MASK_AUTOCOMMIT ;
      }
   }

   void _dpsTransConfItem::setTransAutoRollback( BOOLEAN autoRollback,
                                                 BOOLEAN enableMask )
   {
      if ( _transAutoRollback != autoRollback )
      {
         _transAutoRollback = autoRollback ;
         ++_transConfVer ;
      }
      if ( enableMask )
      {
         _transConfMask |= TRANS_CONF_MASK_AUTOROLLBACK ;
      }
   }

   void _dpsTransConfItem::updateByMask( const _dpsTransConfItem &rhs )
   {
      UINT32 rhsMask = rhs.getTransConfMask() ;
      UINT32 oldTransConfVer = _transConfVer ;

      if ( rhsMask & TRANS_CONF_MASK_ISOLATION )
      {
         setTransIsolation( rhs.getTransIsolation(), TRUE ) ;
      }
      if ( rhsMask & TRANS_CONF_MASK_TIMEOUT )
      {
         setTransTimeout( rhs.getTransTimeout(), TRUE ) ;
      }
      if ( rhsMask & TRANS_CONF_MASK_USERBS )
      {
         setUseRollbackSemgent( rhs.useRollbackSegment(), TRUE ) ;
      }
      if ( rhsMask & TRANS_CONF_MASK_AUTOCOMMIT )
      {
         setTransAutoCommit( rhs.isTransAutoCommit(), TRUE ) ;
      }
      if ( rhsMask & TRANS_CONF_MASK_AUTOROLLBACK )
      {
         setTransAutoRollback( rhs.isTransAutoRollback(), TRUE ) ;
      }
      if ( rhsMask & TRANS_CONF_MASK_WAITLOCK )
      {
         setTransWaitLock( rhs.isTransWaitLock(), TRUE ) ;
      }

      if ( oldTransConfVer != _transConfVer )
      {
         _transConfVer = oldTransConfVer + 1 ;
      }
   }

   void _dpsTransConfItem::copyFrom( const _dpsTransConfItem &rhs )
   {
      *this = rhs ;
   }

   void _dpsTransConfItem::toBson( BSONObjBuilder & builder ) const
   {
      try
      {
         builder.append( FIELD_NAME_TRANSISOLATION, _transIsolation ) ;
         builder.append( FIELD_NAME_TRANS_TIMEOUT,
                         _transTimeout / OSS_ONE_SEC ) ;
         builder.appendBool( FIELD_NAME_TRANS_USE_RBS, _useRollbackSegment ) ;
         builder.appendBool( FIELD_NAME_TRANS_WAITLOCK, _transWaitLock ) ;
         builder.appendBool( FIELD_NAME_TRANS_AUTOCOMMIT, _transAutoCommit ) ;
         builder.appendBool( FIELD_NAME_TRANS_AUTOROLLBACK,
                             _transAutoRollback ) ;
      }
      catch ( std::exception &e )
      {
         /// ignore
         PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
      }
   }

   void _dpsTransConfItem::fromBson( const BSONObj &obj )
   {
      UINT32 oldTransConfVer = _transConfVer ;

      try
      {
         BSONObjIterator itr( obj ) ;
         while( itr.more() )
         {
            BSONElement e = itr.next() ;

            if ( 0 == ossStrcmp( e.fieldName(),
                                 FIELD_NAME_TRANSISOLATION ) )
            {
               setTransIsolation( e.numberInt(), TRUE ) ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(),
                                      FIELD_NAME_TRANS_TIMEOUT ) )
            {
               setTransTimeout( e.numberInt() * OSS_ONE_SEC, TRUE ) ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(),
                                      FIELD_NAME_TRANS_USE_RBS ) )
            {
               setUseRollbackSemgent( e.booleanSafe(), TRUE ) ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(),
                                      FIELD_NAME_TRANS_WAITLOCK ) )
            {
               setTransWaitLock( e.booleanSafe(), TRUE ) ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(),
                                      FIELD_NAME_TRANS_AUTOCOMMIT ) )
            {
               setTransAutoCommit( e.booleanSafe(), TRUE ) ;
            }
            else if ( 0 == ossStrcmp( e.fieldName(),
                                      FIELD_NAME_TRANS_AUTOROLLBACK ) )
            {
               setTransAutoRollback( e.booleanSafe(), TRUE ) ;
            }
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "Occur exeption: %s", e.what() ) ;
         /// ignore
      }

      if ( oldTransConfVer != _transConfVer )
      {
         _transConfVer = oldTransConfVer + 1 ;
      }
   }

   /*
      _dpsTransExecutor implement
   */
   _dpsTransExecutor::_dpsTransExecutor()
   {
      _waiter           = NULL;
      _waiterQueType    = DPS_QUE_NULL ;
      _lastLRB          = NULL;
      _lockCount        = 0 ;

      _useTransLock     = TRUE ;
      _reservedLogSpace = 0 ;
   }

   _dpsTransExecutor::~_dpsTransExecutor()
   {
   }

   void _dpsTransExecutor::clearAll()
   {
      clearWaiterInfo() ;
      clearLastLRB() ;
      clearLock() ;
      clearLockCount() ;
      clearRecordMap() ;
      resetLogSpace() ;
   }

   void _dpsTransExecutor::assertLocks()
   {
      SDB_ASSERT( _mapCSCLLockID.size() == 0, "Lock must be 0" ) ;
      SDB_ASSERT( _lockCount == 0, "Lock must be 0" ) ;
      SDB_ASSERT( _waiter == NULL,
                  "Waiter LRB must be invalid" ) ;
      SDB_ASSERT( _lastLRB == NULL,
                  "Last LRB must be invalid" ) ;
      SDB_ASSERT( isRecordMapEmpty(), "Record map must be empty" ) ;
      SDB_ASSERT( _reservedLogSpace == 0, "Reserved log space must be 0" ) ;
   }

   void _dpsTransExecutor::setWaiterInfo( dpsTransLRB* waiter,
                                          DPS_TRANS_QUE_TYPE type )
   {
      _waiter        = waiter ;
      _waiterQueType = type ;
   }

   void _dpsTransExecutor::clearWaiterInfo()
   {
      _waiter        = NULL;
      _waiterQueType = DPS_QUE_NULL ;
   }

   dpsTransLRB* _dpsTransExecutor::getWaiterLRB() const
   {
      return _waiter ;
   }

   DPS_TRANS_QUE_TYPE _dpsTransExecutor::getWaiterQueType() const
   {
      return _waiterQueType ;
   }

   void _dpsTransExecutor::setLastLRB( dpsTransLRB* lrb )
   {
      _lastLRB = lrb ;
   }

   void _dpsTransExecutor::clearLastLRB()
   {
      _lastLRB = NULL ;
   }

   dpsTransLRB * _dpsTransExecutor::getLastLRB() const
   {
      return _lastLRB ;
   }

   BOOLEAN _dpsTransExecutor::addLock( const dpsTransLockId &lockID,
                                       dpsTransLRB * lrb )
   {
      // only add CS or CL lock into the map
      if ( ! lockID.isLeafLevel() )
      {
         if ( _mapCSCLLockID.insert( std::make_pair( lockID, lrb ) ).second )
         {
            return TRUE ;
         }
      }
      return FALSE ;
   }

   BOOLEAN _dpsTransExecutor::findLock( const dpsTransLockId &lockID,
                                        dpsTransLRB * &lrb ) const
   {
      // only search the map if it is CS or CL lock
      if ( ! lockID.isLeafLevel() )
      {
         DPS_LOCKID_MAP_CIT cit = _mapCSCLLockID.find( lockID ) ;
         if ( cit != _mapCSCLLockID.end() )
         {
            lrb = cit->second ;
            return TRUE ;
         }
      }
      return FALSE ;
   }

   BOOLEAN _dpsTransExecutor::removeLock( const dpsTransLockId &lockID )
   {
      // only do the remove if it is CS or CL lock
      if ( ! lockID.isLeafLevel() )
      {
         return _mapCSCLLockID.erase( lockID ) ? TRUE : FALSE ;
      }
      return FALSE ;
   }

   void _dpsTransExecutor::clearLock()
   {
      _mapCSCLLockID.clear() ;
   }

   void _dpsTransExecutor::incLockCount()
   {
      ++_lockCount ;
   }

   void _dpsTransExecutor::decLockCount()
   {
      SDB_ASSERT( _lockCount > 0, "Lock count must > 0" ) ;
      if ( _lockCount > 0 )
      {
         --_lockCount ;
      }
   }

   void _dpsTransExecutor::clearLockCount()
   {
      _lockCount = 0 ;
   }

   UINT32 _dpsTransExecutor::getLockCount() const
   {
      return _lockCount ;
   }

   BOOLEAN _dpsTransExecutor::useTransLock() const
   {
      return _useTransLock ;
   }

   void _dpsTransExecutor::setUseTransLock( BOOLEAN use )
   {
      _useTransLock = use ;
   }

   void _dpsTransExecutor::initTransConf( INT32 isolation,
                                          UINT32 timeout,
                                          BOOLEAN waitLock,
                                          BOOLEAN autoCommit,
                                          BOOLEAN autoRollback,
                                          BOOLEAN useRBS )
   {
      _transConfMask = 0 ;

      setTransIsolation( isolation, FALSE ) ;
      setTransTimeout( timeout, FALSE ) ;
      setTransWaitLock( waitLock, FALSE ) ;
      setTransAutoCommit( autoCommit, FALSE ) ;
      setTransAutoRollback( autoRollback, FALSE ) ;
      setUseRollbackSemgent( useRBS, FALSE ) ;

      _useTransLock        = TRUE ;
      _transConfVer        = 1 ;
   }

   BOOLEAN _dpsTransExecutor::updateTransConf( INT32 isolation,
                                               UINT32 timeout,
                                               BOOLEAN waitLock,
                                               BOOLEAN autoCommit,
                                               BOOLEAN autoRollback,
                                               BOOLEAN useRBS )
   {
      UINT32 oldTransConfVer = _transConfVer ;
      BOOLEAN updateAll = FALSE ;

      /// only timeout can update in transaction
      if ( !OSS_BIT_TEST( _transConfMask, TRANS_CONF_MASK_TIMEOUT ) )
      {
         setTransTimeout( timeout, FALSE ) ;
      }

      if ( DPS_INVALID_TRANS_ID == getExecutor()->getTransID() )
      {
         if ( !OSS_BIT_TEST( _transConfMask, TRANS_CONF_MASK_ISOLATION ) )
         {
            setTransIsolation( isolation, FALSE ) ;
         }
         if ( !OSS_BIT_TEST( _transConfMask, TRANS_CONF_MASK_WAITLOCK ) )
         {
            setTransWaitLock( waitLock, FALSE ) ;
         }
         if ( !OSS_BIT_TEST ( _transAutoCommit, TRANS_CONF_MASK_AUTOCOMMIT ) )
         {
            setTransAutoCommit( autoCommit, FALSE ) ;
         }
         if ( !OSS_BIT_TEST ( _transAutoRollback, TRANS_CONF_MASK_AUTOROLLBACK ) )
         {
            setTransAutoRollback( autoRollback, FALSE ) ;
         }
         if ( !OSS_BIT_TEST( _transConfMask, TRANS_CONF_MASK_USERBS ) )
         {
            setUseRollbackSemgent( useRBS, FALSE ) ;
         }
         updateAll = TRUE ;
      }

      if ( oldTransConfVer != _transConfVer )
      {
         _transConfVer = oldTransConfVer + 1 ;
      }
      return updateAll ;
   }

   const _dpsTransExecutor::MAP_LSN_2_RECORD*
      _dpsTransExecutor::getRecordMap() const
   {
      return &_mapLSN2Record ;
   }

   void _dpsTransExecutor::putRecord( DPS_LSN_OFFSET lsnOffset,
                                      const dmsRecordID &item )
   {
      pair<MAP_LSN_2_RECORD_IT,BOOLEAN> ret ;
      try
      {
         ret = _mapLSN2Record.insert( MAP_LSN_2_RECORD::value_type( lsnOffset,
                                                                    item ) ) ;
         if ( !ret.second )
         {
            SDB_ASSERT( FALSE, "Item must not been existed" ) ;
            ret.first->second = item ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
      }
   }

   void _dpsTransExecutor::delRecord( DPS_LSN_OFFSET lsnOffset )
   {
      _mapLSN2Record.erase( lsnOffset ) ;
   }

   BOOLEAN _dpsTransExecutor::getRecord( DPS_LSN_OFFSET lsnOffset,
                                         dmsRecordID &item,
                                         BOOLEAN withDel )
   {
      MAP_LSN_2_RECORD_IT it = _mapLSN2Record.find( lsnOffset ) ;
      if ( it != _mapLSN2Record.end() )
      {
         item = it->second ;
         if ( withDel )
         {
            _mapLSN2Record.erase( it ) ;
         }
         return TRUE ;
      }
      return FALSE ;
   }

   void _dpsTransExecutor::clearRecordMap()
   {
      _mapLSN2Record.clear() ;
   }

   BOOLEAN _dpsTransExecutor::isRecordMapEmpty() const
   {
      return _mapLSN2Record.empty() ? TRUE : FALSE ;
   }

   UINT32 _dpsTransExecutor::getRecordMapSize() const
   {
      return _mapLSN2Record.size() ;
   }

   void  _dpsTransExecutor::addReservedSpace( const UINT64 len )
   {
      _reservedLogSpace += len ;
   }

   UINT64 _dpsTransExecutor::getReservedSpace() const 
   { 
      return _reservedLogSpace ; 
   }

   void  _dpsTransExecutor::resetLogSpace()
   {
      _reservedLogSpace = 0 ;
   }

}

