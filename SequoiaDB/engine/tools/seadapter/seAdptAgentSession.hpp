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

   Source File Name = seAdptAgentSession.hpp

   Descriptive Name = Agent session on search engine adapter.

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for sdbcm,
   which is used to do cluster managing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/14/2017  YSD  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SEADPT_AGENT_SESSION_HPP_
#define SEADPT_AGENT_SESSION_HPP_

#include <map>
#include "pmdAsyncSession.hpp"
#include "utilCommObjBuff.hpp"
#include "seAdptMgr.hpp"
#include "utilESClt.hpp"
#include "seAdptContext.hpp"

using namespace engine ;

namespace seadapter
{
   class _seAdptAgentSession : public _pmdAsyncSession
   {
      DECLARE_OBJ_MSG_MAP()
      typedef std::map<INT64, seAdptContextBase *>   CTX_MAP ;
      typedef CTX_MAP::iterator                      CTX_MAP_ITR ;
   public:
      _seAdptAgentSession( UINT64 sessionID ) ;
      virtual ~_seAdptAgentSession() ;

      virtual EDU_TYPES eduType() const ;
      virtual SDB_SESSION_TYPE sessionType() const ;
      virtual const CHAR* className() const { return "SEAdptAgent" ; }

      virtual void onRecieve( const NET_HANDLE netHandle, MsgHeader * msg ) ;
      virtual BOOLEAN timeout( UINT32 interval ) ;
      virtual void onTimer( UINT64 timerID, UINT32 interval ) ;

   protected:
      virtual void _onAttach() ;
      virtual void _onDetach() ;

      INT32 _onOPMsg ( NET_HANDLE handle, MsgHeader * msg ) ;

      INT32 _onQueryReq( MsgHeader *msg,
                         utilCommObjBuff &objBuff,
                         INT64 &contextID,
                         pmdEDUCB *eduCB = NULL ) ;

      INT32 _onGetmoreReq( MsgHeader *msg,
                           utilCommObjBuff &objBuff,
                           INT64 &contextID,
                           pmdEDUCB *eduCB = NULL ) ;

      INT32 _onKillCtxReq( MsgHeader *msg, pmdEDUCB *eduCB = NULL ) ;

      INT32 _reply( MsgOpReply *header, NET_HANDLE handle,
                    const CHAR *buff, UINT32 size ) ;
      INT32 _defaultMsgFunc ( NET_HANDLE handle, MsgHeader *msg ) ;
      INT32 _selectIndex( const CHAR *clName, const BSONObj &hint,
                          BSONObj &newHint, UINT16 &indexID ) ;

   private:
      CTX_MAP           _ctxMap ;
      BSONObj           _errorInfo ;
      INT64             _contextIDHWM ;
   } ;
   typedef _seAdptAgentSession seAdptAgentSession ;
}

#endif /* SEADPT_AGENT_SESSION_HPP_ */

