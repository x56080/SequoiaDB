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

   Source File Name = clsTimerHandler.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          1/12/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_TIMER_HANDLER_HPP_
#define CLS_TIMER_HANDLER_HPP_

#include "pmdAsyncHandler.hpp"

namespace engine
{
   class _pmdAsycSessionMgr ;

   /*
      _clsReplTimerHandler define
   */
   class _clsReplTimerHandler : public _pmdAsyncTimerHandler
   {
      public:
         _clsReplTimerHandler ( _pmdAsycSessionMgr * pSessionMgr ) ;
         virtual ~_clsReplTimerHandler () ;

      protected:
         virtual UINT64  _makeTimerID( UINT32 timerID ) ;

   } ;
   typedef _clsReplTimerHandler clsReplTimerHandler ;

   /*
      _clsShardTimerHandler define
   */
   class _clsShardTimerHandler : public _pmdAsyncTimerHandler
   {
      public:
         _clsShardTimerHandler ( _pmdAsycSessionMgr * pSessionMgr ) ;
         virtual ~_clsShardTimerHandler () ;

      protected:
         virtual UINT64  _makeTimerID( UINT32 timerID ) ;
   } ;
   typedef _clsShardTimerHandler clsShardTimerHandler ;

}

#endif //CLS_TIMER_HANDLER_HPP_

