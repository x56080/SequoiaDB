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

   Source File Name = coordGTSAgent.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   global transaction control in COORD.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORD_GTS_AGENT_HPP__
#define COORD_GTS_AGENT_HPP__

#include "oss.hpp"
#include "dpsDef.hpp"
#include "ossMemPool.hpp"
#include "dpsTransDef.hpp"
#include "sdbInterface.hpp"
#include "dpsGTSAgent.hpp"

namespace engine
{

   class _coordResource ;

   /*
      _coordGTSAgent define
    */
   // _coordGTSAgent handles GTS agent in COORD
   class _coordGTSAgent : public SDBObject,
                          public dpsGTSAgent
   {
   public:
      // constructor and destructor
      _coordGTSAgent() ;
      virtual ~_coordGTSAgent() ;

   public:
      // initialize GTS agent with COORD resource
      INT32 init( _coordResource *resource ) ;
      // finalize GTS agent to release COORD resource
      void  fini() ;

   public:
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
      // WARNING: should not be called for COORD
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
      // WARNINGL should not be called for COORD
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
      // WARNINGL should not be called for COORD
      virtual INT32 waitArbitChange( pmdEDUCB *eduCB,
                                     const DPS_TRANS_ID &arbitTransID,
                                     DPS_TRANS_STATUS currentStatus,
                                     INT32 timeout,
                                     dpsTransBackInfo &newInfo ) ;

      // on attach event
      virtual void   onAttach( pmdEDUCB *eduCB ) ;
      // on detach event
      virtual void   onDetach( pmdEDUCB *eduCB ) ;

   protected:
      // COORD resource
      _coordResource * _resource ;
   } ;

   typedef class _coordGTSAgent coordGTSAgent ;

}

#endif // COORD_GTS_AGENT_HPP__
