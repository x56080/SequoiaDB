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

   Source File Name = schedTaskAdapter.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/19/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SCHED_TASK_ADAPTER_HPP__
#define SCHED_TASK_ADAPTER_HPP__

#include "schedTaskAdapterBase.hpp"
#include "schedTaskQue.hpp"
#include "schedTaskContainer.hpp"
#include "ossAtomic.hpp"
#include "ossEvent.hpp"
#include "ossLatch.hpp"

namespace engine
{

   /*
      _schedFIFOAdapter define
   */
   class _schedFIFOAdapter : public _schedTaskAdapterBase
   {
      public:
         _schedFIFOAdapter() ;
         virtual ~_schedFIFOAdapter() ;

      protected:

         virtual INT32        _onInit( SCHED_TASK_QUE_TYPE queType ) ;
         virtual void         _onFini() ;

         virtual UINT32       _onPrepared( UINT32 expectNum ) ;

         virtual INT32        _onPush( const pmdEDUEvent &event,
                                       UINT64 recvTimeUs,
                                       INT64 priority,
                                       const schedInfo *pInfo ) ;

         virtual SCHED_TYPE   _getType () const { return _type ; }

      private:
         schedTaskQueBase        *_pTaskQue ;
         SCHED_TYPE              _type ;

   } ;
   typedef _schedFIFOAdapter schedFIFOAdapter ;

   /*
      _schedContainerAdapter define
   */
   class _schedContainerAdapter : public _schedTaskAdapterBase
   {
      public:
         _schedContainerAdapter( schedTaskContanierMgr *pMgr ) ;
         virtual ~_schedContainerAdapter() ;

      protected:

         virtual INT32        _onInit( SCHED_TASK_QUE_TYPE queType ) ;
         virtual void         _onFini() ;

         virtual UINT32       _onPrepared( UINT32 expectNum ) ;

         virtual INT32        _onPush( const pmdEDUEvent &event,
                                       UINT64 recvTimeUs,
                                       INT64 priority,
                                       const schedInfo *pInfo ) ;

         virtual SCHED_TYPE   _getType () const
         {
            return SCHED_TYPE_CONTAINER ;
         }

      private:
         schedTaskContanierMgr            *_pMgr ;

   } ;
   typedef _schedContainerAdapter schedContainerAdapter ;

}

#endif // SCHED_TASK_ADAPTER_HPP__
