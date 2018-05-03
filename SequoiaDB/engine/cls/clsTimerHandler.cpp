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

   Source File Name = clsTimerHandler.cpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          1/12/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "core.hpp"
#include "clsBase.hpp"
#include "clsTimerHandler.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"

namespace engine
{

   /*
      _clsReplTimerHandler implement
   */
   _clsReplTimerHandler::_clsReplTimerHandler ( _pmdAsycSessionMgr * pSessionMgr )
      :_pmdAsyncTimerHandler ( pSessionMgr )
   {
   }

   _clsReplTimerHandler::~_clsReplTimerHandler ()
   {
   }

   UINT64 _clsReplTimerHandler::_makeTimerID( UINT32 timerID )
   {
      return ossPack32To64( CLS_REPL, timerID ) ;
   }

   /*
      _clsShardTimerHandler implement
   */
   _clsShardTimerHandler::_clsShardTimerHandler ( _pmdAsycSessionMgr *pSessionMgr )
      :_pmdAsyncTimerHandler ( pSessionMgr )
   {
   }

   _clsShardTimerHandler::~_clsShardTimerHandler ()
   {
   }

   UINT64 _clsShardTimerHandler::_makeTimerID( UINT32 timerID )
   {
      return ossPack32To64( CLS_SHARD, timerID ) ;
   }

}

