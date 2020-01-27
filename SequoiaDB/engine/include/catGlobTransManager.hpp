/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = catGlobTransManager.hpp

   Descriptive Name = Catalog Global Transaction Manager

   When/how to use: this program may be used on binary and text-formatted
   versions of STP component. This file contains structure for global
   transaction manager in Catalog node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CAT_GLOB_TRANS_MANAGER_HPP_
#define CAT_GLOB_TRANS_MANAGER_HPP_

#include "oss.hpp"
#include "ossUtil.hpp"
#include "pmdEDU.hpp"
#include "dpsTransID.hpp"
#include "ossRWMutex.hpp"
#include "ossMemPool.hpp"
#include "utilPooledObject.hpp"

#include "../bson/bson.hpp"

namespace engine
{

   /*
      _catLowTranRecord define
    */
   // _catLowTranRecord holds a lowTran record for a COORD or DATA node
   class _catLowTranRecord : public utilPooledObject
   {
   public:
      // constructor and destructor
      _catLowTranRecord() ;
      _catLowTranRecord( const _catLowTranRecord &record ) ;
      ~_catLowTranRecord() ;

      _catLowTranRecord &operator =( const _catLowTranRecord &record ) ;

   public:
      // initialize transaction node
      // input:
      //    - routeID: route ID of node
      // NOTE: only COORD and DATA nodes are needed
      void initNode( const MsgRouteID &routeID ) ;

      // update node lowTran
      // input:
      //    - lowTran: lowTran reported by node
      //    - expireTran: expireTran reported by node
      //    - transOn: indicate whether transaction feature is enabled on node
      //    - globTransOn: indicate whether global transaction feature is
      //                   enabled on node
      //    - mvccOn: indicate whether MVCC feature is enabled on node
      //    - stpAvailable: indicate whether STP is available on node
      // NOTE: global transaction support requires transOn, mvccOn and
      //       globTransOn must be TRUE
      void updateLowTran( DPS_TRANSID_SN lowTran,
                          DPS_TRANSID_SN expireTran,
                          BOOLEAN transOn,
                          BOOLEAN globTransOn,
                          BOOLEAN mvccOn,
                          BOOLEAN stpAvailable ) ;

      // get node lowTran and expireTran
      // output:
      //    - lowTran: lowTran last reported by this node
      //    - expireTran: expireTran last reported by this node
      // NOTE:
      // - DPS_INVALID_TRANSID_SN: means this node had not reported yet
      // - DPS_MAX_TRANSID_SN: means global transaction support is not enabled
      //                       in this node, or this node had been kicked out
      // - if node had not reported for 2 minutes, it will be kicked out
      //   from lowTran calculation ( consider it is down or disconnected )
      void getLowTran( DPS_TRANSID_SN &lowTran,
                       DPS_TRANSID_SN &expireTran ) ;

   protected:
      // role of node ( COORD or DATA )
      SDB_ROLE       _role ;
      // route ID of node
      MsgRouteID     _routeID ;
      // lowTran of node: minimum running transaction on node
      DPS_TRANSID_SN _lowTran ;
      // expireTran of node: maximum expired transaction on node
      // NODE: transaction objects ( RBS, old version, etc ) before expireTran
      //       could be cleared
      DPS_TRANSID_SN _expireTran ;
      // indicate if global transaction feature is enabled on node
      BOOLEAN        _globTransEnabled ;
      // --transactionon options on node
      BOOLEAN        _transOn ;
      // --globtranson options on node
      BOOLEAN        _globTransOn ;
      // --mvccon options on node
      BOOLEAN        _mvccOn ;
      // indicate if STP is available on node
      BOOLEAN        _stpAvailable ;
      // last update tick for node
      UINT64         _updateTick ;
   } ;

   typedef class _catLowTranRecord catLowTranRecord ;

   /*
      _catGlobTransManager define
    */
   // _catGlobTransManager collects lowTran from COORD and DATA nodes,
   // and calculate global lowTran
   class _catGlobTransManager : public utilPooledObject
   {
   public:
      // constructor and destructor
      _catGlobTransManager() ;
      ~_catGlobTransManager() ;

   public:
      // indicate the lowTran map is expired
      OSS_INLINE void setLowTranMapExpired()
      {
         ossScopedRWLock lock( &_lowTranMutex, EXCLUSIVE ) ;
         _lowTranMapLoaded = FALSE ;
      }

      // clear lowTran
      void clearGlobLowTran() ;

      // get global lowTran
      DPS_TRANSID_SN getGlobLowTran() ;

      // update global lowTran with lowTran from specified node
      // after update, will re-calculate global lowTran
      INT32 updateGlobLowTran( const MsgRouteID &nodeRID,
                               DPS_TRANSID_SN lowTran,
                               DPS_TRANSID_SN expireTran,
                               BOOLEAN transOn,
                               BOOLEAN globTransOn,
                               BOOLEAN mvccOn,
                               BOOLEAN stpAvailable,
                               DPS_TRANSID_SN &globLowTran,
                               DPS_TRANSID_SN &globExpireTran ) ;

   protected:
      typedef ossPoolMap< MsgRouteID,
                          catLowTranRecord,
                          MsgRouteIDComp > GTS_LOWTRAN_MAP ;
      typedef ossPoolSet< MsgRouteID, MsgRouteIDComp > GTS_NODE_SET ;

      // update lowTran of specified node
      // WARNING: should be accessed under _lowTranMutex
      INT32 _updateNodeLowTran( const MsgRouteID &nodeRID,
                                DPS_TRANSID_SN lowTran,
                                DPS_TRANSID_SN expireTran,
                                BOOLEAN transOn,
                                BOOLEAN globTransOn,
                                BOOLEAN mvccOn,
                                BOOLEAN stpAvailable ) ;

      // calculate global lowTran
      // WARNING: should be accessed under _lowTranMutex
      INT32 _calcGlobLowTran( DPS_TRANSID_SN &globLowTran,
                              DPS_TRANSID_SN &globExpireTran ) ;

      // load transaction nodes ( including COORD and DATA )
      INT32 _loadTransNodes( GTS_NODE_SET &transNodes ) ;

      // merge transaction nodes ( check new and removed nodes )
      // WARNING: should be accessed under _lowTranMutex
      INT32 _mergeTransNodes( const GTS_NODE_SET &transNodes ) ;

   protected:
      // mutex to protect global lowTran and lowTran map
      ossRWMutex        _lowTranMutex ;
      // global low transaction ( minimum running transaction )
      DPS_TRANSID_SN    _globLowTran ;
      // global expire transaction ( maximum expired transaction )
      DPS_TRANSID_SN    _globExpireTran ;
      // low transaction from nodes
      GTS_LOWTRAN_MAP   _lowTranMap ;
      // indicate lowTranMap is loaded
      BOOLEAN           _lowTranMapLoaded ;
   } ;

   typedef class _catGlobTransManager catGlobTransManager ;

}

#endif // CAT_GLOB_TRANS_MANAGER_HPP_
