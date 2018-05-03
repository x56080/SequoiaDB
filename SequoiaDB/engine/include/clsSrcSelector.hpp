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

   Source File Name = clsSrcSelector.hpp

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

#ifndef CLSSRCSELECTOR_HPP_
#define CLSSRCSELECTOR_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "msg.hpp"
#include "clsDef.hpp"
#include <set>

using namespace std ;

namespace engine
{
   class _clsSyncManager ;
   class _clsReplicateSet ;
   class _clsNodeMgrAgent ;

   class _clsSrcSelector : public SDBObject
   {
   public:
      _clsSrcSelector() ;
      ~_clsSrcSelector() ;

   public:
      const MsgRouteID &getFullSyncSrc() ;

      const MsgRouteID &getSyncSrc() ;

      const MsgRouteID &selected( BOOLEAN isFullSync = FALSE ) ;

      const MsgRouteID &selectPrimary ( UINT32 groupID,
                                        MSG_ROUTE_SERVICE_TYPE type =
                                        MSG_ROUTE_SHARD_SERVCIE ) ;

      OSS_INLINE void addToBlakList( const MsgRouteID &id )
      {
         _blacklist.insert( id.value ) ;
      }

      OSS_INLINE void clearBlacklist()
      {
         _blacklist.clear() ;
      }

      OSS_INLINE void clearSrc()
      {
         _src.value = MSG_INVALID_ROUTEID ;
         _noRes = 0 ;
      }

      OSS_INLINE void timeout( UINT32 timeout )
      {
         _noRes += timeout ;
      }

      OSS_INLINE void clearTime()
      {
         _noRes = 0 ;
      }

      OSS_INLINE const MsgRouteID &src()
      {
         return _src ;
      }
   private:
      set<UINT64>       _blacklist ;
      _clsSyncManager   *_syncmgr ;
      MsgRouteID        _src ;
      UINT32            _noRes ;
      _clsNodeMgrAgent  *_nodeMgrAgent ;

   } ;
   typedef class _clsSrcSelector clsSrcSelector ;
}

#endif

