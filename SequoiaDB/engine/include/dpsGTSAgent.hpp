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

   Source File Name = dpsGTSAgent.hpp

   Descriptive Name = DPS Global Transaction Agent

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains background job to update
   global lowTran.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPS_GTS_AGENT_HPP__
#define DPS_GTS_AGENT_HPP__

#include "rtnBackgroundJobBase.hpp"
#include "msgCatalog.hpp"
#include "msgReplicator.hpp"
#include "dpsTransID.hpp"
#include "dpsTransCB.hpp"
#include "../bson/bson.hpp"

namespace engine
{

   /*
      _dpsGTSAgent define
    */
   // _dpsGTSAgent use to handle global transaction events
   class _dpsGTSAgent
   {
   public:
      _dpsGTSAgent() ;
      virtual ~_dpsGTSAgent() ;

   public:
      // update global lowTran
      // return:
      //    - SDB_OK: update succeed
      //    - SDB_GLOB_LOWTRAN_UNKNOWN: global lowTran is not ready
      // NOTE: will send local lowTran to catalog and get back global lowTran
      virtual INT32 updateGlobLowTran() = 0 ;

      // arbitrate global transaction
      // input:
      //    - eduCB: EDUCB of current read transaction
      //    - readTransID: transaction ID of read transaction
      //    - writeTransID: transaction ID of write transaction
      //    - writeTransStatus: transaction status of write transaction
      //    - forceLocal: force do arbitration on local
      // output:
      //    - visible: indicate if current read transaction could see changes
      //               from write transaction
      // return:
      //    - SDB_OK: succeed to finish arbitration
      //    - other errors: failed to finish arbitration
      // NOTE:
      //    - only when write transaction is involved in a single DATA
      //      group, we could use the force local mode
      //    - generally, visible will be TRUE when transaction status of
      //      write transaction is committed
      virtual INT32 arbitGlobTrans( pmdEDUCB *eduCB,
                                    const DPS_TRANS_ID &readTransID,
                                    const DPS_TRANS_ID &writeTransID,
                                    DPS_TRANS_STATUS writeTransStatus,
                                    BOOLEAN forceLocal,
                                    BOOLEAN &visible ) = 0 ;

      // wait arbitrating transaction to commit
      // input:
      //    - eduCB: EDUCB of current transaction
      //    - arbitTransID: transaction ID of arbitrating write transaction
      //    - timeout: timeout to wait ( in milliseconds )
      // output:
      //    - committed: indicate if the waiting transaction has committed
      //    - multiGroups: transaction is involved in multiple DATA groups
      //    - commiteTime: commit time of transaction
      // return:
      //    - SDB_OK: succeed to wait result
      //    - SDB_TIMEOUT: timeout to wait result
      //    - other errors: failed to wait result
      virtual INT32 waitArbitCommit( pmdEDUCB *eduCB,
                                     const DPS_TRANS_ID &arbitTransID,
                                     INT32 timeout,
                                     BOOLEAN &committed,
                                     BOOLEAN &multiGroups,
                                     stpLogicalTimeUS &commitTime ) = 0 ;

      // wait arbitrating transaction to change status
      // input:
      //    - eduCB: EDUCB of current transaction
      //    - arbitTransID: transaction ID of arbitrating write transaction
      //    - timeout: timeout to wait ( in milliseconds )
      // output:
      //    - newInfo: transaction info after status changed
      // return:
      //    - SDB_OK: succeed to wait result
      //    - SDB_TIMEOUT: timeout to wait result
      //    - other errors: failed to wait result
      virtual INT32 waitArbitChange( pmdEDUCB *eduCB,
                                     const DPS_TRANS_ID &arbitTransID,
                                     DPS_TRANS_STATUS currentStatus,
                                     INT32 timeout,
                                     dpsTransBackInfo &newInfo ) = 0 ;

      // on attach event
      OSS_INLINE virtual void onAttach( pmdEDUCB *eduCB ) {}
      // on detach event
      OSS_INLINE virtual void onDetach( pmdEDUCB *eduCB ) {}

      // set route ID of local
      OSS_INLINE void setLocalRID( const MsgRouteID &routeID )
      {
         _localRID.value = routeID.value ;
      }

      // get route ID of local
      OSS_INLINE const MsgRouteID &getLocalRID() const
      {
         return _localRID ;
      }

      // get maximum acceptable time error
      UINT32 getMaxNodeTimeError() const
      {
         return _maxNodeTimeError ;
      }

      // set maximum acceptable time error
      // NOTE: this value is in nanosecond, --globtransmaxtimeerror is in
      //       microsecond, need convert
      void setMaxNodeTimeError( UINT32 maxTimeError )
      {
         _maxNodeTimeError = maxTimeError ;
      }

   protected:
      // get local lowTran
      // output:
      //    - localLowTran: local lowTran
      //    - localExpireTran: local expireTran
      // return:
      //    - SDB_OK: succeed to get lowTran
      //    - other error: failed to get lowTran
      // NOTE: if no global transaction in this node, will return max value
      //       of transaction SN in local lowTran
      INT32 _getLocalLowTran( DPS_TRANSID_SN &localLowTran,
                              DPS_TRANSID_SN &localExpireTran ) ;
      // set global lowTran
      // input:
      //    - globLowTran: global lowTran
      //    - globExpireTran: global expireTran
      // return:
      //    - SDB_OK: succeed to set lowTran
      //    - other error: failed to set lowTran
      INT32 _setGlobLowTran( const DPS_TRANSID_SN &globLowTran,
                             const DPS_TRANSID_SN &globExpireTran ) ;

      // fill GTS lowTran request
      // input:
      //    - request: GTS lowTran request
      //    - localLowTran: local lowTran
      //    - localExpireTran: local expireTran
      // output:
      //    - requestObject: BSON object contains local lowTran and local
      //                     expireTran
      // return:
      //    - SDB_OK: succeed to fill request
      //    - other errors: failed to fill request
      INT32 _fillLowTranReq( MsgGTSLowTranReq *request,
                             const DPS_TRANSID_SN &localLowTran,
                             const DPS_TRANSID_SN &localExpireTran,
                             bson::BSONObj &requestObject ) ;

      // parse GTS lowTran response
      // input:
      //    - response: GTS lowTran response
      // output:
      //    - globLowTran: global lowTran extracted from response
      //    - globExpireTran: global expireTran extracted from response
      // return:
      //    - SDB_OK: succeed to parse response
      //    - other errors: failed to parse response
      INT32 _parseLowTranRsp( MsgGTSLowTranRsp *response,
                              DPS_TRANSID_SN &globLowTran,
                              DPS_TRANSID_SN &globExpireTran ) ;

      // fill GTS arbitrate request
      // input:
      //    - request: GTS arbitrate request
      //    - readTransID: transaction ID of read transaction
      //    - writeTransID: transaction ID of write transaction
      //    - writeTransStatus: transaction status of write transaction
      // return:
      //    - SDB_OK: succeed to fill request
      //    - other errors: failed to fill request
      INT32 _fillGTSArbitReq( MsgClsGTSArbitReq *request,
                              const DPS_TRANS_ID &readTransID,
                              const DPS_TRANS_ID &writeTransID,
                              DPS_TRANS_STATUS writeTransStatus ) ;

      // parse GTS arbitrate response
      // input:
      //    - response: GTS arbitrate response
      // output:
      //    - visible: arbitration result to indicate if current read
      //               transaction could see changes from write transaction
      // return:
      //    - SDB_OK: succeed to parse response
      //    - other errors: failed to parse response
      INT32 _parseGTSArbitRsp( const MsgClsGTSArbitRsp *response,
                               BOOLEAN &visible ) ;

   protected:
      // pointer to transCB
      dpsTransCB *   _transCB ;

      // route ID of local node
      // NOTE: should be set after register to CATALOG
      MsgRouteID     _localRID ;

      // maximum acceptable time error ( --globtransmaxtimeerror )
      // NOTE: this value is in nanosecond, --globtransmaxtimeerror is in
      //       microsecond, need convert
      volatile UINT32      _maxNodeTimeError ;
   } ;

   typedef class _dpsGTSAgent dpsGTSAgent ;

   /*
       _dpsGTSLowTranJob define
    */
   class _dpsGTSLowTranJob : public _rtnBaseJob
   {
   public:
      _dpsGTSLowTranJob( dpsGTSAgent *gtsAgent ) ;
      virtual ~_dpsGTSLowTranJob() ;

   public:
      OSS_INLINE virtual RTN_JOB_TYPE type() const
      {
         return RTN_JOB_GTS_LOWTRAN ;
      }

      OSS_INLINE virtual const CHAR *name() const
      {
         return "GTSLowTranJob" ;
      }

      OSS_INLINE virtual BOOLEAN muteXOn( const _rtnBaseJob *other )
      {
         // only one lowTran job is allowed
         return ( other->type() == RTN_JOB_GTS_LOWTRAN ) ? TRUE : FALSE ;
      }

      virtual INT32 doit() ;

   protected:
      OSS_INLINE virtual void _onAttach() ;
      OSS_INLINE virtual void _onDetach() ;

   protected:
      // GTS agent
      dpsGTSAgent * _gtsAgent ;
   } ;

   typedef class _dpsGTSLowTranJob dpsGTSLowTranJob ;

   /*
      function to start GTS lowTran job
    */
   INT32 dpsStartGTSLowTranJob( dpsGTSAgent *gtsAgent, EDUID *eduID ) ;

}

#endif // DPS_GTS_AGENT_HPP__
