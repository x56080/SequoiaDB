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

   Source File Name = seAdptMsgHandler.hpp

   Descriptive Name = Search Engine Adapter Message Handler.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/04/2017  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SEADPT_MSG_HANDLER_HPP__
#define SEADPT_MSG_HANDLER_HPP__

#include "core.hpp"
#include "pmdAsyncHandler.hpp"
#include "pmdAsyncSession.hpp"

using namespace engine ;

namespace seadapter
{
   class _indexMsgHandler : public _pmdAsyncMsgHandler
   {
      public:
         _indexMsgHandler( pmdAsycSessionMgr *pSessionMgr ) ;
         virtual ~_indexMsgHandler() ;

         virtual void handleClose( const NET_HANDLE &handle,
                                   _MsgRouteID id ) ;
   } ;
   typedef _indexMsgHandler indexMsgHandler ;
}

#endif /* SEADPT_MSG_HANDLER_HPP__ */

