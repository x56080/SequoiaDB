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

   Source File Name = catGTSMsgHandler.hpp

   Descriptive Name = GTS message handler

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/17/2018  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CAT_GTS_MSG_HANDLER_HPP_
#define CAT_GTS_MSG_HANDLER_HPP_

#include "oss.hpp"
#include "ossUtil.hpp"
#include "ossQueue.hpp"
#include "ossLatch.hpp"
#include "netDef.hpp"
#include "msg.h"
#include "rtnContextBuff.hpp"

namespace engine
{
   struct _catGTSMsg ;
   class sdbCatalogueCB ;
   class _catGTSManager ;
   class _pmdEDUCB ;

   class _catGTSMsgHandler: public SDBObject
   {
   public:
      _catGTSMsgHandler() ;
      ~_catGTSMsgHandler() ;

      INT32 init() ;
      INT32 fini() ;

      INT32 active() ;
      INT32 deactive() ;

      void checkLoad() ;
      void jobExit( BOOLEAN isController ) ;

      INT32 postMsg( const NET_HANDLE& handle, const MsgHeader* msg ) ;

      BOOLEAN popMsg( INT64 timeout, _catGTSMsg*& gtsMsg ) ;

      INT32 processMsg( const _catGTSMsg* gtsMsg ) ;

      INT32 primaryCheck() ;

      INT32 sendReply ( const NET_HANDLE& handle,
                        MsgOpReply* reply,
                        void* replyData = NULL,
                        UINT32 replyDataLen = 0 ) ;

   private:
      INT32 _ensureMsgJobController() ;
      INT32 _processSequenceAcquireMsg( MsgHeader* msg, _pmdEDUCB* eduCB,
                                        rtnContextBuf& buf ) ;
      INT32 _processSequenceCreateMsg( MsgHeader* msg, _pmdEDUCB* eduCB ) ;
      INT32 _processSequenceDropMsg( MsgHeader* msg, _pmdEDUCB* eduCB,
                                     rtnContextBuf& buf ) ;
      INT32 _processSequenceAlterMsg( MsgHeader* msg, _pmdEDUCB* eduCB,
                                      rtnContextBuf& buf ) ;

   private:
      _catGTSManager*         _gtsMgr ;
      ossQueue<_catGTSMsg*>   _msgQueue ;
      sdbCatalogueCB*         _catCB ;

      ossSpinXLatch           _jobLatch ;
      INT32                   _activeJobNum ;
      INT32                   _maxJobNum ;
      BOOLEAN                 _isControllerStarted ;

      MsgRouteID              _primaryID ;
   } ;
   typedef _catGTSMsgHandler catGTSMsgHandler ;
}

#endif /* CAT_GTS_MSG_HANDLER_HPP_ */
