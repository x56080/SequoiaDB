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

      // pre-arbitrate global write transaction
      // input:
      //    - writeTransID: transaction ID of current write transaction
      //    - preArbitList: list of pre-arbitrate read transactions
      // return:
      //    - SDB_OK: succeed to do pre-arbitration
      //    - other errors: failed to do pre-arbitration
      // NOTE: writeTransID should be original transaction ID without tags
      //       except for global transaction tag
      // WARNINGL should not be called for COORD
      virtual INT32 preArbitGlobTrans( const DPS_TRANS_ID writeTransID,
                                       TRANS_ID_LIST &preArbitList ) ;

      // wait arbitrating transaction to commit
      // input:
      //    - eduCB: EDUCB of current transaction
      //    - arbitTransID: transaction ID of arbitrating write transaction
      //    - timeout: timeout to wait ( in milliseconds )
      // output:
      //    - committed: indicate if the waiting transaction has committed
      //    - multiGroups: transaction is involved in multiple DATA groups
      // return:
      //    - SDB_OK: succeed to wait result
      //    - SDB_TIMEOUT: timeout to wait result
      //    - other errors: failed to wait result
      // WARNINGL should not be called for COORD
      virtual INT32 waitArbitCommit( pmdEDUCB *eduCB,
                                     const DPS_TRANS_ID &transID,
                                     INT32 timeout,
                                     BOOLEAN &commited,
                                     BOOLEAN &multiGroups ) ;

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
