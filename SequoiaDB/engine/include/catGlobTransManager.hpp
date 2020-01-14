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
      _catGlobTransManager define
    */
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

      // get global lowTran
      DPS_TRANSID_SN getGlobLowTran() ;

      // update global lowTran with lowTran from specified node
      // after update, will re-calculate global lowTran
      INT32 updateGlobLowTran( const MsgRouteID &nodeRID,
                               DPS_TRANSID_SN nodeLowTran,
                               DPS_TRANSID_SN &globLowTran ) ;

   protected:
      typedef ossPoolMap< UINT64, DPS_TRANSID_SN > GTS_LOWTRAN_MAP ;
      typedef ossPoolSet< UINT64 > GTS_NODE_SET ;

      // update lowTran of specified node
      // WARNING: should be accessed under _lowTranMutex
      INT32 _updateNodeLowTran( const MsgRouteID &nodeRID,
                                DPS_TRANSID_SN nodeLowTran ) ;

      // calculate global lowTran
      // WARNING: should be accessed under _lowTranMutex
      INT32 _calcGlobLowTran( DPS_TRANSID_SN &globLowTran ) ;

      // load transaction nodes ( including COORD and DATA )
      INT32 _loadTransNodes( GTS_NODE_SET &transNodes ) ;

      // merge transaction nodes ( check new and removed nodes )
      // WARNING: should be accessed under _lowTranMutex
      INT32 _mergeTransNodes( const GTS_NODE_SET &transNodes ) ;

   protected:
      // mutex to protect global lowTran and lowTran map
      ossRWMutex        _lowTranMutex ;
      // global low transaction ( earliest running transaction )
      DPS_TRANSID_SN    _globLowTran ;
      // low transaction from nodes
      GTS_LOWTRAN_MAP   _lowTranMap ;
      // indicate lowTranMap is loaded
      BOOLEAN           _lowTranMapLoaded ;
   } ;

   typedef class _catGlobTransManager catGlobTransManager ;

}

#endif // CAT_GLOB_TRANS_MANAGER_HPP_
