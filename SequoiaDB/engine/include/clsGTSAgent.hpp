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

   Source File Name = clsGTSAgent.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   replication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/05/2012  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CLS_GTS_AGENT_HPP__
#define CLS_GTS_AGENT_HPP__

#include "oss.hpp"
#include "dpsDef.hpp"
#include "ossMemPool.hpp"
#include "dpsTransDef.hpp"
#include "sdbInterface.hpp"
#include "dpsGTSAgent.hpp"
#include "dpsLogWrapper.hpp"

namespace engine
{
   class _clsShardMgr ;

   /*
      _clsGTSAgent define
   */
   class _clsGTSAgent : public SDBObject,
                        public _dpsTransEvent,
                        public dpsGTSAgent
   {
      public:
         _clsGTSAgent( _clsShardMgr *pShardMgr ) ;
         virtual ~_clsGTSAgent() ;

      public:
         // rollback all running transactions
         virtual INT32  onRollbackAll() ;

         // update global lowTran
         // return:
         //    - SDB_OK: update succeed
         //    - SDB_GLOB_LOWTRAN_UNKNOWN: global lowTran is not ready
         // NOTE: will send local lowTran to catalog and get back global lowTran
         virtual INT32 updateGlobLowTran() ;

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
                                       BOOLEAN &visible ) ;

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
                                        stpLogicalTimeUS &commitTime ) ;

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
                                        dpsTransBackInfo &newInfo ) ;

         // get node time error
         UINT32      getNodeTimeError() ;

         // try increase node time error
         // input:
         //    - currentTimeError: current time error
         void        incNodeTimeError( UINT32 currentTimeError ) ;

         // try decrease node time error
         // input:
         //    - currentTimeError: current time error
         void        decNodeTimeError( UINT32 currentTimeError ) ;

         // reset node time error
         void        resetNodeTimeError() ;

         // get accept time error between remote time and local time
         // NOTE: result ( in nanosecond ) is used to adjust node time error
         UINT32      getAcceptTimeError( const stpLogicalTimeUS &remoteTime,
                                         const stpLogicalTimeUS &localTime ) ;

      public:
         // check transaction status in other nodes
         // input:
         //    - transID: transaction ID of checking transaction
         //    - nodeNum: number of nodes to be checked
         //    - pNodes: nodes to be checked
         //    - cb: EDUCB of current transaction
         //    - preCommitTime: pre-commit time of transaction on current node
         // output:
         //    - status: transaction status of other nodes ( indicates whether
         //              this transaction should be committed )
         //    - commitTime: commit time of transaction
         // return:
         //    - SDB_OK: succeed to check status
         //    - other errors: failed to check status
         // NOTE:
         //    - the status of transaction in this node is wait-commit or
         //      in-doubt
         //    - nodes are involved for this transaction
         INT32       checkTransStatus( DPS_TRANS_ID transID,
                                       UINT32 nodeNum,
                                       const UINT64 *pNodes,
                                       IExecutor *cb,
                                       UINT64 preCommitTime,
                                       DPS_TRANS_STATUS &status,
                                       UINT64 &commitTime ) ;

      protected:
         // check transaction status in other groups
         // input:
         //    - transID: transaction ID of checking transaction
         //    - nodeNum: number of nodes to be checked
         //    - pNodes: nodes to be checked
         //    - cb: EDUCB of current transaction
         //    - checkForArbit: if check for arbitrate, it will return when
         //                     transaction is found committed or rollbacked
         //                     in one node
         //    - preCommitTime: pre-commit time of transaction on current node
         // output:
         //    - status: transaction status of other nodes ( indicates whether
         //              this transaction should be committed )
         //    - commitTime: commit time of transaction
         // return:
         //    - SDB_OK: succeed to check status
         //    - other errors: failed to check status
         // NOTE:
         //    - the status of transaction in this node is wait-commit or
         //      in-doubt
         //    - nodes are involved for this transaction
         //    - for arbitration, we need call with TRUE checkForArbit
         //    - for 2-phase commit check, we need to call with FALSE
         //      checkForArbit, we need to acquire all status of involved
         //      nodes
         INT32       _checkTransStatus( DPS_TRANS_ID transID,
                                        UINT32 nodeNum,
                                        const UINT64 *pNodes,
                                        IExecutor *cb,
                                        BOOLEAN checkForArbit,
                                        UINT64 preCommitTime,
                                        DPS_TRANS_STATUS &status,
                                        UINT64 &commitTime ) ;

         // check transaction status in a given group
         // input:
         //    - transID: transaction ID of checking transaction
         //    - group: group ID to send check request
         //    - cb: EDUCB of current transaction
         // output:
         //    - status: transaction status of other nodes ( indicates whether
         //              this transaction should be committed )
         //    - preCommitTime: global logical time to pre-commit transaction
         //    - commitTime: global logical time to commit transaction
         // return:
         //    - SDB_OK: succeed to check status
         //    - other errors: failed to check status
         // NOTE:
         //    - the status of transaction in this node is wait-commit or
         //      in-doubt
         //    - the group are involved for this transaction ( since the
         //      original node might be down, so we need check with other nodes
         //      in the same group )
         INT32       _checkTransStatus( DPS_TRANS_ID transID,
                                        UINT32 group,
                                        IExecutor *cb,
                                        DPS_TRANS_STATUS &status,
                                        UINT64 &preCommitTime,
                                        UINT64 &commitTime ) ;

         // get commit info from DPS log
         // input:
         //    - commitLSN: LSN to write the commit DPS log
         // output:
         //    - mb: DPS message block to store the record
         //    - logType: logType of record found by LSN
         //    - attr: commit attribute ( pre-commit or final commit )
         //    - nodeNum: number of nodes/groups involved in this transaction
         //    - nodes: nodes/groups involved in this transaction
         // return:
         //    - SDB_OK: succeed to get commit info
         //    - other errors: failed to get commit info
         // NOTE:
         //    for DPS record loaded from file, its memory will be
         //    held in DPS message block, so we need to keep DPS
         //    message block to access the fields in DPS record
         //    especailly for the array of nodes
         INT32       _getCommitInfo( DPS_LSN_OFFSET commitLSN,
                                     dpsMessageBlock *mb,
                                     DPS_LOG_TYPE &logType,
                                     UINT8 &attr,
                                     UINT32 &nodeNum,
                                     const UINT64 **nodes ) ;

         // check wait-commit in-doubt transaction status
         // input:
         //    - transID: transaction ID of wait-commit transaction
         //    - curLsn: current LSN of transaction ( pre-commit record )
         // output:
         //    - status: status of transaction after check
         //    - preCommitTime: global logical time to pre-commit transaction
         //    - commitTime: global logical time to commit transaction
         // return:
         //    - SDB_OK: succeed to check transaction status
         //    - other errors: failed to check transaction status
         INT32       _syncCheckTransStatus( DPS_TRANS_ID transID,
                                            DPS_LSN_OFFSET curLsn,
                                            DPS_TRANS_STATUS &status,
                                            UINT64 preCommitTime,
                                            UINT64 &commitTime ) ;

         // commit transaction
         // input:
         //    - transID: transaction ID of committing transaction
         //    - lastLsn: last LSN of transaction
         //    - commitTime: commit time of transaction
         // output:
         //    - curLsn: LSN of commit record
         // return:
         //    - SDB_OK: succeed to commit transaction
         //    - other errors: failed to commit transaction
         INT32       _commitTrans( DPS_TRANS_ID transID,
                                   DPS_LSN_OFFSET lastLsn,
                                   UINT64 commitTime,
                                   DPS_LSN_OFFSET &curLsn ) ;

         // arbitrate transactions on remote node
         // input:
         //    - eduCB: EDUCB of read transaction
         //    - readTransID: transaction ID of read transaction
         //    - writeTransID: transaction ID of write transaction
         //    - writeTransStatus: transaction status of write transaction
         // output:
         //    - visible: indicate if read transaction could see changes
         //               from write transaction
         // return:
         //    - SDB_OK: succeed to arbitrate transaction
         //    - other error: failed to arbitrate transaction
         // NOTE:
         //    - remote node is the node to launch read transaction, it should
         //      be a COORD node
         //    - only when write transaction is committed, the visible could be
         //      TRUE
         INT32       _arbitRemote( pmdEDUCB *eduCB,
                                   const DPS_TRANS_ID &readTransID,
                                   const DPS_TRANS_ID &writeTransID,
                                   DPS_TRANS_STATUS writeTransStatus,
                                   BOOLEAN &visible ) ;

         // arbitrate transactions on local node
         // input:
         //    - eduCB: EDUCB of read transaction
         //    - readTransID: transaction ID of read transaction
         //    - writeTransID: transaction ID of write transaction
         //    - writeTransStatus: transaction status of write transaction
         // output:
         //    - visible: indicate if read transaction could see changes
         //               from write transaction
         // return:
         //    - SDB_OK: succeed to arbitrate transaction
         //    - other error: failed to arbitrate transaction
         // NOTE:
         //    - local node is the node to launch read transaction, it should
         //      be a DATA node
         //    - only when write transaction is committed, the visible could be
         //      TRUE
         INT32       _arbitLocal( pmdEDUCB *eduCB,
                                  const DPS_TRANS_ID &readTransID,
                                  const DPS_TRANS_ID &writeTransID,
                                  DPS_TRANS_STATUS writeTransStatus,
                                  BOOLEAN &visible ) ;

      private:
         _clsShardMgr         *_pShardMgr ;
         // latch to protect node time error
         ossSpinSLatch        _timeErrorLatch ;
         // current node time error in nanosecond
         // NOTE: note time error indicates the accepted time error caused
         //       by network delay via TCP connections between transaction
         //       nodes ( COORD ) and current node ( DATA )
         //       since the STP time error only consider network delay between
         //       STP synchronization client and source via UDP connection,
         //       while the transaction message between transaction nodes are
         //       using TCP connections, so the network delay might be larger
         UINT32               _nodeTimeError ;
         // count to decrease node time error
         UINT32               _decTimeErrorCount ;
   } ;
   typedef _clsGTSAgent clsGTSAgent ;

}


#endif // CLS_GTS_AGENT_HPP__

