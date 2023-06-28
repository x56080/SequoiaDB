/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
                      BSONObj &errorObj ) ;
   INT32 _processMsg( const CHAR *pMsg, BSONObj &errorObj ) ;

   INT32 _onMsgBegin( MsgHeader *pMsg ) ;
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
   BOOLEAN _shouldBuildGetMoreMsg( const _mongoCommand *pCommand ) ;

   INT32   _processClientMsg( const CHAR* pMsg,
                              _mongoCommand *&pCommand,
                              mongoSessionCtx &sessCtx ) ;

   INT32   _processOwnedClientMsg( const CHAR* pMsg,
                                   mongoMsgBuffer *pSdbMsgBuff,
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
} ;

typedef _mongoSession mongoSession ;

}
#endif // _SDB_MONGO_MSG_CONVERTER_HPP_
