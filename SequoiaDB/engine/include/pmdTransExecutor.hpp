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

   Source File Name = pmdTransExecutor.hpp

   Descriptive Name = Operating System Services Types Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/08/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef PMD_TRANS_EXECUTOR_HPP__
#define PMD_TRANS_EXECUTOR_HPP__

#include "dpsTransExecutor.hpp"

namespace engine
{

   class _pmdEDUCB ;

   /*
      _pmdTransExecutor define
   */
   class _pmdTransExecutor : public _dpsTransExecutor
   {
      public:
         _pmdTransExecutor( _pmdEDUCB *cb, monMonitorManager *monMgr ) ;
         virtual ~_pmdTransExecutor() ;

      public:
         /*
            Interface
         */
         virtual EDUID        getEDUID() const ;
         virtual UINT32       getTID() const ;
         virtual void         wakeup( INT32 wakeupRC ) ;
         virtual INT32        wait( INT64 timeout ) ;
         virtual IExecutor*   getExecutor() ;
         virtual BOOLEAN      isInterrupted () ;

      protected:
         _pmdEDUCB            *_pEDUCB ;

   } ;
   typedef _pmdTransExecutor pmdTransExecutor ;

}

#endif // PMD_TRANS_EXECUTOR_HPP__

