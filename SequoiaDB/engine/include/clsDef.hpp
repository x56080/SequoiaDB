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

   Source File Name = clsDef.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   replication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLSDEF_HPP_
#define CLSDEF_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "dpsLogDef.hpp"
#include "netDef.hpp"
#include "msgDef.h"
#include "ossMem.hpp"
#include "msg.h"
#include "pmdEDU.hpp"
#include "ossRWMutex.hpp"
#include "dms.hpp"
#include "utilReplSizePlan.hpp"
#include "utilLocation.hpp"

#include <map>

using namespace std ;

namespace engine
{
   typedef UINT32 CLS_GROUP_VERSION ;

   const UINT32 CLS_VOTE_CS_TIME = 3000 ;
   const UINT32 CLS_SHARING_BETA_INTERVAL = 2000 ;
   const UINT32 CLS_SYNC_MAX_LEN = 1024 * 1024 * 5 ;
   const UINT32 CLS_SYNC_DETAIL_MAX_LEN = 1024 ;
   extern INT32  g_startShiftTime ;

   #define CLS_TID(sessionid)          ((UINT32)(sessionid & 0xFFFFFFFF))
   #define CLS_NODEID(sessionid)       ((UINT32)(sessionid >> 32))

   #define CLS_BEATID_BEGIN                     ( 1 )
   #define CLS_BEATID_INVALID                   ( 0 )

   //full sync node timeout
   #define CLS_FS_NORES_TIMEOUT                 (10000)  // 10 secs
   #define CLS_DST_SESSION_NO_MSG_TIME          (300000) // 5 mins
   #define CLS_SRC_SESSION_NO_MSG_TIME          (10000)  // 10 secs

   #define CLS_FS_MAX_BSON_SIZE                 ( 14 * 1024 * 1024 )

   // Temporary weight, use for reelect / fault tolerant
   #define CLS_ELECTION_WEIGHT_MIN 0
   #define CLS_ELECTION_WEIGHT_MAX 101

   // Default weight
   #define CLS_ELECTION_WEIGHT_USR_MIN 1
   #define CLS_ELECTION_WEIGHT_USR_MAX 100

   #define CLS_GET_WEIGHT( weight, shadowWeight )\
           (( CLS_ELECTION_WEIGHT_MIN == (shadowWeight) ) ?\
              (shadowWeight) :\
            ( (weight) < (shadowWeight) ?\
              (shadowWeight) : (weight) ))

   // The weight should be the power of 2, and in range [0, 128]
   // The macro can be use to represent election weight as well as election weight mask
   #define CLS_ELECTION_WEIGHT_DFT                     0x00
   #define CLS_ELECTION_WEIGHT_REELECT_TARGET_NODE     0x80
   #define CLS_ELECTION_WEIGHT_ACTIVE_LOCATION         0x08
   #define CLS_ELECTION_WEIGHT_AFFINITIVE_LOCATION     0x04

   enum CLS_SYNC_STATUS
   {
      CLS_SYNC_STATUS_NONE = 0,
      CLS_SYNC_STATUS_PEER = 1,
      CLS_SYNC_STATUS_RC = 2,
   } ;

   enum CLS_BEAT_STATUS
   {
      CLS_BEAT_STATUS_BREAK = 0,
      CLS_BEAT_STATUS_ALIVE = 1,
   } ;

   const UINT32 CLS_BEAT_ID_OVERTURN_WINDOW = 2 ^ 31 ;

   enum CLS_GROUP_ROLE
   {
      CLS_GROUP_ROLE_SECONDARY = 0,
      CLS_GROUP_ROLE_PRIMARY =1,
   } ;

   enum CLS_ELECTION_STATUS
   {
      CLS_ELECTION_STATUS_SILENCE = 0,
      CLS_ELECTION_STATUS_SEC = 1,
      CLS_ELECTION_STATUS_VOTE = 2,
      CLS_ELECTION_STATUS_ANNOUNCE = 3,
      CLS_ELECTION_STATUS_PRIMARY = 4,
   } ;

   enum CLS_NODE_SERVICE_STATUS
   {
      SERVICE_NORMAL          = 0,     /// node is normal, and data is normal
      SERVICE_ABNORMAL,                /// node is normal, but data is abnormal
      SERVICE_UNKNOWN                  /// node is abnormal(crashed)
   } ;

   enum CLS_NODE_RUN_STAT
   {
      CLS_NODE_RUNNING  = 0,
      CLS_NODE_CATCHUP  = 1,
      CLS_NODE_STOP     = 2
   } ;

   /*
      CLS_SELECT_RANGE define
   */
   enum CLS_SELECT_RANGE
   {
      CLS_SELECT_BEGIN = 0,
      // select the location set node
      CLS_SELECT_LOCATION,
      // select the affinity location set node
      CLS_SELECT_AFFINITY_LOCATION,
      // select the group node
      CLS_SELECT_GROUP,
      CLS_SELECT_END
   } ;

   // after 5 times without receiving message, mark the UDP unavailable
   #define CLS_UDP_UNAVAILABLE  ( 5 )

   #define CLS_BEAT_VERSION_1       ( 1 )
   #define CLS_BEAT_VERSION_2       ( 2 )

   /*
      _clsGroupBeat define
   */
   class _clsGroupBeat : public SDBObject
   {
   public :
      DPS_LSN                 endLsn ;
      _MsgRouteID             identity ;
      UINT8                   weight ;
      UINT8                   beatVersion ;
      UINT8                   nodeRunStat ;
      UINT8                   electionWeight ;
      CHAR                    hashCode[4] ;
      CLS_GROUP_VERSION       version ;
      CLS_GROUP_ROLE          role ;         // self role
      CLS_SYNC_STATUS         syncStatus ;
      UINT32                  beatID ;
      CLS_NODE_SERVICE_STATUS serviceStatus ;

      /*
         >= CLS_BEAT_VERSION_1
      */
      UINT32                  ftConfirmStat ;
      INT32                   indoubtErr ;

      /*
         >= CLS_BEAT_VERSION_2
      */
      UINT32                  locationID ;
      CLS_GROUP_ROLE          locationRole ;
      UINT8                   locationWeight ;

      _clsGroupBeat(): version( 0 ),
                       role( CLS_GROUP_ROLE_SECONDARY ),
                       syncStatus( CLS_SYNC_STATUS_NONE ),
                       beatID( CLS_BEATID_INVALID ),
                       serviceStatus( SERVICE_UNKNOWN )
      {
         weight = 0 ;
         beatVersion = CLS_BEAT_VERSION_2 ;
         nodeRunStat = (UINT8)CLS_NODE_RUNNING ;
         electionWeight = CLS_ELECTION_WEIGHT_DFT ;

         ftConfirmStat = 0 ;
         indoubtErr = SDB_OK ;

         locationID = MSG_INVALID_LOCATIONID ;
         locationRole = CLS_GROUP_ROLE_SECONDARY ;
         locationWeight = 0 ;
      }

      UINT8 getElectionWeight() const
      {
         return beatVersion >= CLS_BEAT_VERSION_2 ? electionWeight : CLS_ELECTION_WEIGHT_DFT ;
      }

      UINT32 getLocationID() const
      {
         if ( beatVersion >= CLS_BEAT_VERSION_2 )
         {
            return locationID ;
         }
         return MSG_INVALID_LOCATIONID ;
      }

      UINT32 getLocationRole() const
      {
         if ( beatVersion >= CLS_BEAT_VERSION_2 )
         {
            return locationRole ;
         }
         return CLS_GROUP_ROLE_SECONDARY ;
      }

      UINT32 getLocationWeight() const
      {
         if ( beatVersion >= CLS_BEAT_VERSION_2 )
         {
            return locationWeight ;
         }
         return CLS_ELECTION_WEIGHT_USR_MIN ;
      }

      UINT32 getFTConfirmStat() const
      {
         if ( beatVersion >= CLS_BEAT_VERSION_1 )
         {
            return ftConfirmStat ;
         }
         return 0 ;
      }

      INT32 getIndoubtErr() const
      {
         if ( beatVersion >= CLS_BEAT_VERSION_1 )
         {
            return indoubtErr ;
         }
         return SDB_OK ;
      }

      _clsGroupBeat& operator= ( const _clsGroupBeat &rhs )
      {
         endLsn = rhs.endLsn ;
         identity = rhs.identity ;
         weight = rhs.weight ;
         nodeRunStat = rhs.nodeRunStat ;
         version = rhs.version ;
         role = rhs.role ;
         syncStatus = rhs.syncStatus ;
         beatID = rhs.beatID ;
         serviceStatus = rhs.serviceStatus ;

         ossMemcpy( hashCode, rhs.hashCode, sizeof( hashCode ) ) ;

         if ( rhs.beatVersion >= CLS_BEAT_VERSION_1 )
         {
            beatVersion = rhs.beatVersion ;
            ftConfirmStat = rhs.ftConfirmStat ;
            indoubtErr = rhs.indoubtErr ;
         }
         else
         {
            beatVersion = CLS_BEAT_VERSION_1 ;
            ftConfirmStat = 0 ;
            indoubtErr = SDB_OK ;
         }

         if ( rhs.beatVersion >= CLS_BEAT_VERSION_2 )
         {
            locationID = rhs.locationID ;
            locationRole = rhs.locationRole ;
            locationWeight = rhs.locationWeight ;
            electionWeight = rhs.electionWeight ;
         }
         else
         {
            beatVersion = CLS_BEAT_VERSION_2 ;
            locationID = MSG_INVALID_LOCATIONID ;
            locationRole = CLS_GROUP_ROLE_SECONDARY ;
            locationWeight = 0 ;
            electionWeight = CLS_ELECTION_WEIGHT_DFT ;
         }
         return *this ;
      }

      BOOLEAN isValidID( const UINT32 &id )
      {
         if ( CLS_BEATID_INVALID == beatID )
         {
            return TRUE ;
         }
         else if ( beatID < id )
         {
            return TRUE ;
         }
         else if ( (beatID - id) >
                    CLS_BEAT_ID_OVERTURN_WINDOW )
         {
            return TRUE ;
         }
         else
         {
            return FALSE ;
         }
      }
   } ;

   /*
      _clsSharingStatus define
   */
   class _clsSharingStatus : public SDBObject
   {
   public:
      _clsGroupBeat beat ;
      UINT32        timeout ;
      UINT32        breakTime ;
      UINT32        deadtime ;
      UINT32        sendFailedTimes ;
      UINT32        locationID ;
      BOOLEAN       isAffinitiveLocation ;
      UINT8         locationIndex ;

   protected:
      // need test remote status, which might not support UDP
      // 1. at the beginning, send beat both by UDP and TCP
      // 2. if received UDP message, mark remote UDP supported
      // 3. if not received UDP message after 5 test beats, mark remote
      //    UDP unavailable
      BOOLEAN _supportUDP ;
      INT32   _testUDPCount ;

   public:
      _clsSharingStatus()
      {
         resetStatus() ;
         resetUDP() ;
         isAffinitiveLocation = FALSE ;
         locationID = MSG_INVALID_LOCATIONID ;
         locationIndex = 0xFF ;
      }

      OSS_INLINE BOOLEAN isUDPSupported()
      {
         return _supportUDP ;
      }

      OSS_INLINE BOOLEAN isUDPUnavailable()
      {
         return _testUDPCount > CLS_UDP_UNAVAILABLE ;
      }

      OSS_INLINE void increaseUDPTest()
      {
         ++ _testUDPCount ;
      }

      OSS_INLINE void setUDPSupported()
      {
         _supportUDP = TRUE ;
         _testUDPCount = 0 ;
      }

      OSS_INLINE void setUDPUnavailable()
      {
         _supportUDP = FALSE ;
         _testUDPCount = CLS_UDP_UNAVAILABLE + 1 ;
      }

      OSS_INLINE void resetStatus()
      {
         timeout = 0 ;
         breakTime = 0 ;
         deadtime = 0 ;
         sendFailedTimes = 0 ;
      }

      OSS_INLINE void resetUDP()
      {
         _supportUDP = FALSE ;
         _testUDPCount = 0 ;
      }

      OSS_INLINE BOOLEAN isPrimary( BOOLEAN isLocation )
      {
         return ( isLocation && CLS_GROUP_ROLE_PRIMARY == beat.getLocationRole() ) ||
                ( !isLocation && CLS_GROUP_ROLE_PRIMARY == beat.role ) ;
      }

   } ;

   /*
      _clsLocationInfoItem define
   */
   struct _clsLocationInfoItem
   {
      _clsLocationInfoItem()
      {
         _locationID    = MSG_INVALID_LOCATIONID ;
         _primary.value = 0 ;
         _isAffinitiveLocation = FALSE ;
         _locationIndex = 0xFF ;
      }

      UINT32         _locationID ;
      ossPoolString  _location ;
      MsgRouteID     _primary ;
      BOOLEAN        _isAffinitiveLocation ;
      UINT8          _locationIndex ;
   } ;
   // locationID is key, location info is value
   typedef ossPoolMap< UINT32, _clsLocationInfoItem > CLS_LOC_INFO_MAP ;

   /*
      _clsCatGroupItem define
   */
   struct _clsCatGroupItem
   {
      _clsCatGroupItem()
      {
         groupID = 0 ;
         primary = 0 ;
         secID = 0 ;
         version = 0 ;
      }

      string                     groupName ;
      UINT32                     groupID ;
      UINT32                     primary ;
      UINT32                     secID ;
      CLS_GROUP_VERSION          version ;
      map<UINT64, _netRouteNode> groupInfo ;

      ossPoolString              activeLocation ;
      CLS_LOC_INFO_MAP           locationInfo ;
   } ;
   typedef _clsCatGroupItem      clsCatGroupItem ;

   #define CLS_NODE_KEEPALIVE_TIMEOUT              ( 6000 ) // ms

   /*
      _clsGroupInfo define
   */
   class _clsGroupInfo : public SDBObject
   {
   public :
      map<UINT64, _clsSharingStatus>     info ;
      map<UINT64, _clsSharingStatus *>   alives ;
      ossRWMutex                         mtx ;
      _MsgRouteID                        primary ;
      _MsgRouteID                        local ;
      UINT32                             localBeatID ;
      CLS_GROUP_VERSION                  version ;

      UINT32                             localLocationID ;
      ossPoolString                      localLocation ;
      CLS_LOC_INFO_MAP                   locationInfoMap ;

   private:
      UINT32 _hashCode ;

   public:
      _clsGroupInfo():
      localBeatID( CLS_BEATID_BEGIN ), version( 0 ),
      localLocationID( MSG_INVALID_LOCATIONID ), _hashCode( 0 )
      {
         local.value = 0 ;
         primary.value = 0 ;
      }

      ~_clsGroupInfo()
      {
         alives.clear() ;
         info.clear() ;
         locationInfoMap.clear() ;
      }

      void resetInfo()
      {
         alives.clear() ;
         info.clear() ;
         locationInfoMap.clear() ;
         primary.value = 0 ;
         localBeatID = CLS_BEATID_BEGIN ;
         version = 0 ;
         localLocationID = MSG_INVALID_LOCATIONID ;
         localLocation.clear() ;
      }

      UINT32 nextBeatID()
      {
         ++localBeatID ;
         if ( localBeatID <= CLS_BEATID_BEGIN  )
         {
            localBeatID = CLS_BEATID_BEGIN + 1 ;
         }
         return localBeatID ;
      }

      void     setHashCode( UINT32 hashCode ) { _hashCode = hashCode ; }
      UINT32   getHashCode() const { return _hashCode ; }

      UINT32 groupSize ()
      {
         return info.size() + 1 ;
      }

      UINT32 aliveSize ()
      {
         return alives.size() + 1 ;
      }

      BOOLEAN isAllNodeBeat()
      {
         map<UINT64, _clsSharingStatus>::iterator it = info.begin() ;
         while ( it != info.end() )
         {
            _clsSharingStatus &status = it->second ;
            if ( 0 == status.beat.beatID )
            {
               return FALSE ;
            }
            ++it ;
         }
         return TRUE ;
      }

      BOOLEAN isAllNodeAbnormal( UINT32 timeout )
      {
         map<UINT64, _clsSharingStatus>::iterator it = info.begin() ;
         while ( it != info.end() )
         {
            _clsSharingStatus &status = it->second ;
            if ( SERVICE_NORMAL == status.beat.serviceStatus )
            {
               return FALSE ;
            }
            else if ( SERVICE_UNKNOWN == status.beat.serviceStatus &&
                      ( 0 == timeout || status.breakTime < timeout ) )
            {
               return FALSE ;
            }
            ++it ;
         }
         return TRUE ;
      }

      UINT32 getAlivesByTimeout( UINT32 timeout = CLS_NODE_KEEPALIVE_TIMEOUT )
      {
         UINT32 count = 1 ;
         map<UINT64, _clsSharingStatus>::iterator it = info.begin() ;
         while ( it != info.end() )
         {
            _clsSharingStatus &status = it->second ;
            if ( SERVICE_UNKNOWN == status.beat.serviceStatus &&
                 ( 0 == timeout || status.breakTime > timeout ) )
            {
               ++it ;
               continue ;
            }
            ++count ;
            ++it ;
         }
         return count ;
      }

      UINT32 getNodeSendFailedTimes( UINT64 nodeID )
      {
         map<UINT64, _clsSharingStatus>::iterator it = info.find( nodeID ) ;
         SDB_ASSERT( it != info.end(), "Node is not exist" ) ;

         if ( it != info.end() )
         {
            return it->second.sendFailedTimes ;
         }
         return (UINT32)-1 ;
      }

   } ;

   enum CLS_ELECTION_ROUND
   {
      CLS_ELECTION_ROUND_STAGE_ONE = 0,
      CLS_ELECTION_ROUND_STAGE_TWO = 1,
   } ;

   class _clsSyncSession : public SDBObject
   {
   public :
      utilReplSizePlan waitPlan ;
      _pmdEDUCB *eduCB ;

      /// local write has been completed.
      /// synced starts from one.
      _clsSyncSession():waitPlan(),eduCB( NULL )
      {}

      BOOLEAN operator<( const _clsSyncSession &session ) const
      {
         return waitPlan < session.waitPlan ;
      }

      BOOLEAN operator<=( const _clsSyncSession &session ) const
      {
         return waitPlan <= session.waitPlan ;
      }
   } ;


   #define CLS_SAME_SYNC_LSN_MAX_TIMES    (20)

   /*
      _clsSyncStatus define
   */
   class _clsSyncStatus : public SDBObject
   {
   public :
      DPS_LSN_OFFSET    offset ;
      _MsgRouteID       id ;
      BOOLEAN           valid ;
      UINT32            sameReqTimes ;
      UINT32            locationID ;
      BOOLEAN           affinitive ;
      UINT8             locationIndex ;

      _clsSyncStatus():offset(0)
      {
         id.value       = 0 ;
         valid          = TRUE ;
         sameReqTimes   = 0 ;
         locationID     = MSG_INVALID_LOCATIONID ;
         affinitive     = FALSE ;
         locationIndex  = 0xFF ;
      }

      _clsSyncStatus& operator=( const _clsSyncStatus &right )
      {
         offset         = right.offset ;
         id.value       = right.id.value ;
         valid          = right.valid ;
         sameReqTimes   = right.sameReqTimes ;
         locationID     = right.locationID ;
         affinitive     = right.affinitive ;
         locationIndex  = right.locationIndex ;

         return *this ;
      }

      BOOLEAN isValid() const
      {
         // 1. already full sync
         // 2. sharing-break
         // 3. same sync req more than 20 times
         if ( DPS_INVALID_LSN_OFFSET == offset ||
              !valid ||
              sameReqTimes > CLS_SAME_SYNC_LSN_MAX_TIMES )
         {
            return FALSE ;
         }
         return TRUE ;
      }
   } ;

   /*
      _clsLSNNtyInfo define
   */
   struct _clsLSNNtyInfo
   {
      UINT32               _csLID ;
      UINT32               _clLID ;
      dmsExtentID          _extLID ;
      DPS_LSN_OFFSET       _offset ;

      _clsLSNNtyInfo ()
      {
         _csLID = ~0 ;
         _clLID = ~0 ;
         _extLID = -1 ;
         _offset = 0 ;
      }
      _clsLSNNtyInfo( UINT32 csLID, UINT32 clLID, dmsExtentID extLID,
                      DPS_LSN_OFFSET offset )
      {
         _csLID      = csLID ;
         _clLID      = clLID ;
         _extLID     = extLID ;
         _offset     = offset ;
      }
   } ;
   typedef _clsLSNNtyInfo clsLSNNtyInfo ;

   /*
      _clsReplayEventHandler define
   */
   class _clsReplayEventHandler
   {
      public:
         _clsReplayEventHandler () {}
         virtual ~_clsReplayEventHandler () {}

         virtual void onReplayLog( UINT32 csLID, UINT32 clLID,
                                   INT32 extLID, DPS_LSN_OFFSET offset ) = 0 ;
   } ;

   typedef _clsReplayEventHandler clsReplayEventHandler ;

   enum CLS_REELECTION_LEVEL
   {
      CLS_REELECTION_LEVEL_NONE = 0,
      CLS_REELECTION_LEVEL_1 = 1,
      CLS_REELECTION_LEVEL_3 = 3,  /// wait for at least one replication catch up
      CLS_REELECTION_LEVEL_MAX
   } ;

}

#endif // CLSDEF_HPP_

