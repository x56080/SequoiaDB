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

   Source File Name = schedTaskAdapterBase.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/19/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SCHED_TASK_ADAPTER_BASE_HPP__
#define SCHED_TASK_ADAPTER_BASE_HPP__

#include "netDef.hpp"
#include "schedDef.hpp"
#include "pmdDef.hpp"
#include "schedTaskQue.hpp"

using namespace bson ;

namespace engine
{

   /*
      _schedTaskAdapterBase define
   */
   class _schedTaskAdapterBase : public SDBObject
   {
      public:
         _schedTaskAdapterBase() ;
         virtual ~_schedTaskAdapterBase() ;

         INT32                init( schedTaskInfo *pInfo,
                                    SCHED_TASK_QUE_TYPE queType ) ;

         void                 fini() ;

         BOOLEAN              pop( INT64 millisec,
                                   MsgHeader **pHeader,
                                   NET_HANDLE &handle,
                                   pmdEDUMemTypes &memType,
                                   UINT64 &recvTimeUs ) ;

         INT32                push( const NET_HANDLE &handle,
                                    const _MsgHeader *header,
                                    const schedInfo *pInfo,
                                    UINT64 recvTimeUs ) ;

         UINT32               prepare( INT64 millisec ) ;

         void                 dump( BSONObjBuilder &builder ) ;
         void                 resetDump() ;

      protected:

         virtual INT32        _onInit( SCHED_TASK_QUE_TYPE queType ) = 0 ;
         virtual void         _onFini() = 0 ;

         virtual UINT32       _onPrepared( UINT32 expectNum ) = 0 ;

         virtual INT32        _onPush( const pmdEDUEvent &event,
                                       UINT64 recvTimeUs,
                                       INT64 priority,
                                       const schedInfo *pInfo ) = 0 ;

         virtual SCHED_TYPE   _getType () const = 0 ;

      protected:

         void                 _push2Que( const pmdEDUEvent &event, UINT64 recvTimeUs ) ;
         BOOLEAN              _isControlMsg( const pmdEDUEvent &event ) ;

      protected:
         schedFIFOTaskQue              _queue ;
         schedTaskInfo                 *_pTaskInfo ;

         ossAtomic32                   _doNotify ;
         ossAutoEvent                  _notifyEvent ;

         ossAtomic64                   _hardNum ;
         ossAtomic64                   _eventNum ;
         ossAtomic64                   _cacheNum ;

   } ;
   typedef _schedTaskAdapterBase schedTaskAdapterBase ;

}

#endif // SCHED_TASK_ADAPTER_BASE_HPP__
