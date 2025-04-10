/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = clsSyncManager.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLSSYNCMANAGER_HPP_
#define CLSSYNCMANAGER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "clsDef.hpp"
#include "ossLatch.hpp"
#include "clsSyncMinHeap.hpp"
#include "msgReplicator.hpp"
#include "ossAtomic.hpp"
#include "ossMemPool.hpp"
#include "utilReplSizePlan.hpp"

namespace engine
{
   class _netRouteAgent ;
   class _dpsLogWrapper ;
   class _pmdEDUCB ;

   typedef ossPoolMultiMap<DPS_LSN_OFFSET, utilReplSizePlan>      CLS_WAKE_PLAN ;

   /*
      CLS_NODE_ARRAY define
   */
   struct CLS_NODE_ARRAY
   {
   private:
      UINT64      _nodes[ CLS_REPLSET_MAX_NODE_SIZE - 1 ] ;
      UINT32      _length ;

   public:
      CLS_NODE_ARRAY()
      {
         ossMemset( _nodes, MSG_INVALID_ROUTEID, sizeof(_nodes) ) ;
         _length = 0 ;
      }

      BOOLEAN  addNode( UINT64 value )
      {
         if ( _length < CLS_REPLSET_MAX_NODE_SIZE - 1 )
         {
            _nodes[ _length ] = value ;
            ++_length ;
            return TRUE ;
         }
         return FALSE ;
      }

      UINT64 getNode( UINT32 pos ) const
      {
         if ( pos < _length )
         {
            return _nodes[ pos ] ;
         }
         return MSG_INVALID_ROUTEID ;
      }

      UINT32 getLength() const { return _length ; }
   } ;

   /*
      _clsSyncManager define
   */
   class _clsSyncManager : public SDBObject
   {
   public:
      _clsSyncManager( _netRouteAgent *agent,
                       _clsGroupInfo *info,
                       _clsGroupInfo *locationInfo = NULL ) ;

      ~_clsSyncManager() ;

   public:
      INT32 sync( _clsSyncSession &session,
                  const UINT32 &w,
                  INT64 timeout = -1,
                  BOOLEAN isFTWhole = FALSE ) ;

      void  updateNodeStatus( const MsgRouteID &id, BOOLEAN valid ) ;
      void  notifyFullSync( const MsgRouteID &id ) ;

      INT32 updateNotifyList( BOOLEAN newNodeValid ) ;

      void complete( const MsgRouteID &id,
                     const DPS_LSN &lsn,
                     UINT32 TID ) ;

      void handleTimeout( const UINT32 &interval ) ;

      UINT32 getNotifyNodes( const DPS_LSN_OFFSET &offset, CLS_NODE_ARRAY &nodes ) ;

      void notify( const DPS_LSN_OFFSET &offset ) ;
      void notifyNodes( const CLS_NODE_ARRAY &nodes ) ;

      MsgRouteID getSyncSrc( const set<UINT64> &blacklist,
                             CLS_GROUP_VERSION &version,
                             BOOLEAN useLocaction = TRUE ) ;

      MsgRouteID getFullSrc( const set<UINT64> &blacklist,
                             CLS_GROUP_VERSION &version,
                             BOOLEAN useLocaction = TRUE ) ;

      OSS_INLINE BOOLEAN isReadyToReplay()
      {
         BOOLEAN rc = _blockSync.peek() > 0 ? FALSE : TRUE ;
         if ( rc )
         {
            _info->mtx.lock_r() ;
            rc = ( _info->primary.value == _info->local.value &&
                   _info->primary.value != MSG_INVALID_ROUTEID ) ?
                 FALSE : TRUE ;
            _info->mtx.release_r() ;
         }
         return rc ;
      }

      void  enableSync()
      {
         SDB_ASSERT( _blockSync.peek() > 0, "block sync should be > 0" ) ;
         _blockSync.dec() ;
      }

      void disableSync()
      {
         _blockSync.inc() ;
      }

      void cut( UINT32 alives, BOOLEAN isFTWhole = FALSE ) ;

      DPS_LSN_OFFSET getSyncCtrlArbitLSN() ;

      /// offset is current offset.
      BOOLEAN atLeastOne( const DPS_LSN_OFFSET &offset,
                          UINT16 ensureNodeID = 0,
                          BOOLEAN onlyInAlive = TRUE ) ;

      BOOLEAN isGroupInfoExpired( CLS_GROUP_VERSION version )
      {
         return version < _info->version ;
      }

   private:
      INT32 _wait( _clsSyncSession &session, UINT32 sub,
                   BOOLEAN hasJump, INT64 timeout = -1 ) ;

      /// compare waitPlan of session whit checkList, and if necessary,
      /// jump to next checklist until it passes checklist.
      INT32 _jump( _clsSyncSession &session, UINT32 &sub, BOOLEAN &needWait, BOOLEAN &hasJump ) ;

      void _createWakePlan( CLS_WAKE_PLAN &plan ) ;

      void _wake( CLS_WAKE_PLAN &plan ) ;

      void _complete( const MsgRouteID &id,
                      const DPS_LSN_OFFSET &offset ) ;


      void _clearSyncList( UINT32 removed, UINT32 removedAlives,
                           UINT32 preAlives, UINT32 preSyncNum,
                           _clsSyncStatus *left ) ;

      MsgRouteID _getSyncSrc( const set<UINT64> &blacklist,
                              CLS_SELECT_RANGE rangeType ) ;

      MsgRouteID _getFullSrc( const set<UINT64> &blacklist,
                              CLS_SELECT_RANGE rangeType ) ;

   private:
      /// sub between <0, CLS_REPLSET_MAX_NODE_SIZE - 2>.
      /// means ( w = 2 ) to ( w = CLS_REPLSET_MAX_NODE_SIZE ).
      _clsSyncMinHeap  _syncList[CLS_REPLSET_MAX_NODE_SIZE - 1] ;
      utilReplSizePlan _checkList[CLS_REPLSET_MAX_NODE_SIZE -1] ;
      _ossSpinXLatch   _mtxs[CLS_REPLSET_MAX_NODE_SIZE - 1] ;
      _clsSyncStatus   _notifyList[CLS_REPLSET_MAX_NODE_SIZE - 1] ;

      _netRouteAgent *_agent ;
      _clsGroupInfo *_info ;
      _clsGroupInfo *_locationInfo ;
      MsgRouteID _syncSrc ;

      /// valid _notifyList size
      UINT32 _validSync ;
      UINT32 _timeout ;
      UINT32 _aliveCount ;

      UINT32   _wakeTimeout ;
      // counts of operations which are blocking repl sync
      ossAtomic32 _blockSync ;
   } ;
}

#endif

