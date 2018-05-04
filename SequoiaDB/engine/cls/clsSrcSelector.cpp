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

   Source File Name = clsSrcSelector.cpp

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

#include "clsSrcSelector.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"

namespace engine
{
   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSRCSL__CLSSRCSL, "_clsSrcSelector::_clsSrcSelector" )
   _clsSrcSelector::_clsSrcSelector()
   :_syncmgr( NULL ),
    _noRes( 0 )
   {
      PD_TRACE_ENTRY ( SDB__CLSSRCSL__CLSSRCSL ) ;
      _nodeMgrAgent = sdbGetShardCB()->getNodeMgrAgent() ;
      _syncmgr = sdbGetReplCB()->syncMgr() ;
      _src.value = MSG_INVALID_ROUTEID ;
      PD_TRACE_EXIT ( SDB__CLSSRCSL__CLSSRCSL ) ;
   }

   _clsSrcSelector::~_clsSrcSelector()
   {
      _src.value = MSG_INVALID_ROUTEID ;
      _syncmgr = NULL ;
      _nodeMgrAgent = NULL ;
      _blacklist.clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSRCSL_GETFLSYNSRC, "_clsSrcSelector::getFullSyncSrc" )
   const MsgRouteID &_clsSrcSelector::getFullSyncSrc()
   {
      PD_TRACE_ENTRY ( SDB__CLSSRCSL_GETFLSYNSRC ) ;
      _src = _syncmgr->getFullSrc( _blacklist ) ;
      if ( MSG_INVALID_ROUTEID == _src.value )
      {
         _blacklist.clear() ;
         _src = _syncmgr->getFullSrc( _blacklist ) ;
      }
      PD_TRACE_EXIT ( SDB__CLSSRCSL_GETFLSYNSRC ) ;
      return _src ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSRCSL_GETSYNCSRC, "_clsSrcSelector::getSyncSrc" )
   const MsgRouteID &_clsSrcSelector::getSyncSrc()
   {
      PD_TRACE_ENTRY ( SDB__CLSSRCSL_GETSYNCSRC ) ;
      _src = _syncmgr->getSyncSrc( _blacklist ) ;
      if ( MSG_INVALID_ROUTEID == _src.value )
      {
         _blacklist.clear() ;
         _src = _syncmgr->getSyncSrc( _blacklist ) ;
      }
      PD_TRACE_EXIT ( SDB__CLSSRCSL_GETSYNCSRC ) ;
      return _src ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSRCSL_SLTED, "_clsSrcSelector::selected" )
   const MsgRouteID &_clsSrcSelector::selected( BOOLEAN isFullSync )
   {
      PD_TRACE_ENTRY ( SDB__CLSSRCSL_SLTED ) ;
      MsgRouteID &id =  _src ;

      if ( _noRes > CLS_FS_NORES_TIMEOUT )
      {
         _src.value = MSG_INVALID_ROUTEID ;
      }

      if ( MSG_INVALID_ROUTEID != _src.value )
      {
         goto done ;
      }
      _noRes  = 0 ;

      if ( isFullSync )
      {
         id = getFullSyncSrc() ;
         goto done ;
      }
      else
      {
         id = getSyncSrc() ;
         goto done ;
      }
   done :
      PD_TRACE_EXIT ( SDB__CLSSRCSL_SLTED ) ;
      return id ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSSRCSL_SLPMY, "_clsSrcSelector::selectPrimary" )
   const MsgRouteID &_clsSrcSelector::selectPrimary ( UINT32 groupID,
                                         MSG_ROUTE_SERVICE_TYPE type )
   {
      PD_TRACE_ENTRY ( SDB__CLSSRCSL_SLPMY ) ;
      if ( _noRes > CLS_FS_NORES_TIMEOUT )
      {
         _src.value = MSG_INVALID_ROUTEID ;
      }

      if ( MSG_INVALID_ROUTEID != _src.value )
      {
         goto done ;
      }
      {
         _noRes  = 0 ;

         INT32 rc = SDB_OK ;

         //update group info
         rc = sdbGetShardCB()->syncUpdateGroupInfo( groupID ) ;
         if ( SDB_OK != rc )
         {
            goto done ;
         }

         _nodeMgrAgent->lock_r () ;
         _nodeMgrAgent->groupPrimaryNode( groupID, _src, type ) ;
         _nodeMgrAgent->release_r () ;
      }

   done:
      PD_TRACE_EXIT ( SDB__CLSSRCSL_SLPMY ) ;
      return _src ;
   }

}
