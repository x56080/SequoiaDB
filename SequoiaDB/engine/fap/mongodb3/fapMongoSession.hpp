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

   Source File Name = fapMongoSession.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          07/03/2021  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef _SDB_MONGO_MSG_CONVERTER_HPP_
#define _SDB_MONGO_MSG_CONVERTER_HPP_

#include "dpsLogWrapper.hpp"
#include "rtnContextBuff.hpp"
#include "rtnContext.hpp"
#include "pmdSession.hpp"
#include "fapMongoMessage.hpp"
#include "fapMongoUtil.hpp"
#include "fapMongoCommand.hpp"

namespace fap
{

/*
   _mongoSession define
*/
class _mongoSession : public engine::pmdSession
{
public:
   _mongoSession( SOCKET fd, engine::IResource *pResource ) ;
   virtual ~_mongoSession() ;

   virtual INT32 getServiceType() const ;
   virtual engine::SDB_SESSION_TYPE sessionType() const ;

   virtual INT32 run() ;

protected:
   virtual void  _onAttach() {}
   virtual void  _onDetach() {}

private:
   INT32 _processMsg( const CHAR *pMsg, const _mongoCommand *pCommand,
                      BOOLEAN getMoreAll, BSONObj &errorObj ) ;
   INT32 _processMsg( const CHAR *pMsg, BSONObj &errorObj,
                      engine::rtnContextBuf &buf, MsgOpReply &replyHeader ) ;

   INT32 _onMsgBegin( MsgHeader *pMsg, MsgOpReply &replyHeader ) ;
   void  _onMsgEnd( INT32 result, MsgHeader *pMsg ) ;

   INT32 _recvMsgFromClient( CHAR *&pMsg ) ;

   INT32 _reply( _mongoCommand *pCommand, const CHAR* pMsg,
                 INT32 errCode, BSONObj &errObj ) ;

   void  _resetBuffers() ;
   INT32 _autoCreateCS( const CHAR *pCsName, BSONObj &errorObj ) ;
   INT32 _autoCreateCL( const CHAR *pCSName, const CHAR *pClFullName,
                        BSONObj &errorObj ) ;
   INT32 _autoInsert( const CHAR *pClFullName, const BSONObj &matcher,
                      const BSONObj &updatorObj, const BSONObj &setOnInsert,
                      BSONObj &target, BSONObj &errorObj ) ;
   INT32 _autoKillCursor( UINT64 requestID, INT64 contextID ) ;

   BOOLEAN _shouldAutoCrtCS( const _mongoCommand *pCommand ) ;
   BOOLEAN _shouldAutoCrtCL( const _mongoCommand *pCommand ) ;
   BOOLEAN _shouldBuildGetMoreMsg( const _mongoCommand *pCommand,
                                   const engine::rtnContextBuf &buf,
                                   const MsgOpReply &replyHeader ) ;

   INT32   _processClientMsg( const CHAR* pMsg,
                              _mongoCommand *&pCommand,
                              mongoSessionCtx &sessCtx ) ;

   INT32   _processOwnedClientMsg( const CHAR* pMsg,
                                   mongoMsgBuffer &sdbMsgBuff,
                                   _mongoCommand *pCommand,
                                   mongoSessionCtx &sessCtx,
                                   BOOLEAN &needNext ) ;

   void    _buildErrorObj( const engine::rtnContextBuf &contextBuff,
                           INT32 errCode, BSONObjBuilder &builder ) ;
private:
   void    _saveOrSetMsgGlobalID( MsgHeader *pMsg ) ;
   void    _clearErrorInfo( BSONObj &errObj, mongoSessionCtx *pSessCtx = NULL ) ;

private:
   MsgOpReply              _replyHeader ;
   engine::rtnContextBuf   _contextBuff ;
   mongoMsgBuffer          _inBuffer ;
   mongoMsgBuffer          _tmpBuffer ;
   engine::IResource      *_pResource ;
   const CHAR*             _clFullName ;
   engine::rtnContextStoreBuf    _storeBuff ;
} ;

typedef _mongoSession mongoSession ;

}
#endif // _SDB_MONGO_MSG_CONVERTER_HPP_
