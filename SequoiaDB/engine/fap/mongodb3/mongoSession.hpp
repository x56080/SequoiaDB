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

   Source File Name = mongoSession.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/27/2015  LZ  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef _SDB_MONGO_MSG_CONVERTER_HPP_
#define _SDB_MONGO_MSG_CONVERTER_HPP_

#include "dpsLogWrapper.hpp"
#include "rtnContextBuff.hpp"
#include "pmdSession.hpp"
#include "fapMongoMessage.hpp"
#include "fapMongoUtil.hpp"
#include "fapMongoCommand.hpp"
#include "pmdRemoteSession.hpp"

namespace fap
{

/*
   _mongoSession define
*/
class _mongoSession : public engine::pmdSession, public engine::IRemoteMsgPreprocessor
{
public:
   _mongoSession( SOCKET fd, engine::IResource *resource ) ;
   virtual ~_mongoSession() ;

   virtual INT32 getServiceType() const ;
   virtual engine::SDB_SESSION_TYPE sessionType() const ;

   virtual INT32 run() ;

   virtual BOOLEAN preProcess( engine::pmdEDUEvent &event ) ;

protected:
   virtual void  _onAttach() {}
   virtual void  _onDetach() {}

protected:
   INT32 _processMsg( const CHAR *pMsg, const _mongoCommand *pCommand,
                      BSONObj &errorObj ) ;
   INT32 _processMsg( const CHAR *pMsg, BSONObj &errorObj ) ;
   void  _onMsgBegin( MsgHeader *msg ) ;
   void  _onMsgEnd( INT32 result, MsgHeader *msg ) ;
   INT32 _recvMsg( CHAR *&pMsg,
                   BOOLEAN &recvFromEvent,
                   engine::pmdEDUEvent &event ) ;
   INT32 _recvFromSocket( CHAR *&pMsg, BOOLEAN &recvSomething ) ;
   INT32 _buildResponse( _mongoCommand *pCommand,
                         CHAR *&pRes ) ;
   INT32 _reply( _mongoCommand *pCommand ) ;
   INT32 _reply( _mongoCommand *pCommand, const CHAR* pMsg,
                 INT32 errCode, BSONObj &errObj ) ;
   INT32 _reply( engine::pmdEDUEvent &event ) ;

private:
   void  _resetBuffers() ;
   INT32 _autoCreateCS( const CHAR *csName, BSONObj &errorObj ) ;
   INT32 _autoCreateCL( const CHAR *clFullName, BSONObj &errorObj ) ;
   INT32 _autoInsert( const CHAR *clFullName, const BSONObj &matcher,
                      const BSONObj &updatorObj, BSONObj &target,
                      BSONObj &errorObj ) ;
   INT32 _autoKillCursor( UINT64 requestID, INT64 contextID ) ;

   BOOLEAN _isOwnedCursor( const _mongoCommand *pCommand,
                           UINT64 &ownedEDUID,
                           BOOLEAN &needAuth,
                           INT64 &cursorID ) ;
   INT32 _manageCursor( const _mongoCommand *pCommand,
                        const MsgOpReply &sdbReply ) ;

   BOOLEAN _shouldAutoCrtCS( const _mongoCommand *pCommand ) ;
   BOOLEAN _shouldAutoCrtCL( const _mongoCommand *pCommand ) ;
   BOOLEAN _shouldBuildGetMoreMsg( const _mongoCommand *pCommand ) ;

   void    _postInnerErrorEvent( INT32 errorCode,
                                 engine::pmdEDUEvent &event ) ;
   void    _eduEventRelease( engine::pmdEDUEvent &event ) ;
   void    _buildErrResponseMsg( CHAR* pMsg, INT32 errorCode,
                                 INT32 &msgLen ) ;

private:
   MsgOpReply              _replyHeader ;
   BOOLEAN                 _masterRead ;
   engine::rtnContextBuf   _contextBuff ;

   msgBuffer               _inBuffer ;
   msgBuffer               _tmpBuffer ;
   engine::IResource      *_resource ;

   std::set<INT64>         _cursorList ;
   BOOLEAN                 _needWaitResponse ;

   BOOLEAN                 _isAuthed ;
   queue<engine::pmdEDUEvent> _tmpEventQue ;
   INT32                   _requestIDOfPostEvent ;
   INT32                   _opCodeOfPostEvent ;
   INT64                   _cursorIdOfPostEvent ;
} ;

typedef _mongoSession mongoSession ;

}
#endif // _SDB_MONGO_MSG_CONVERTER_HPP_
