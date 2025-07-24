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
   defect Date        Who         Description
   ====== =========== =========== ==============================================
          01/03/2020  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef _SDB_MONGO_MSG_CONVERTER_HPP_
#define _SDB_MONGO_MSG_CONVERTER_HPP_

#include "util.hpp"
#include "dpsLogWrapper.hpp"
#include "pmdSession.hpp"
#include "mongodef.hpp"
#include "parser.hpp"
#include "mongoConverter.hpp"

/*
   _mongoSession define
*/

class _mongoSession : public engine::pmdSession
{
public:
   _mongoSession( SOCKET fd, engine::IResource *resource ) ;
   virtual ~_mongoSession() ;

   virtual INT32 getServiceType() const ;
   virtual engine::SDB_SESSION_TYPE sessionType() const ;

   virtual INT32 run() ;

protected:
   virtual void  _onAttach() {}
   virtual void  _onDetach() {}

protected:
   BOOLEAN _preProcessMsg( msgParser &parser,
                           engine::IResource *resource,
                           engine::rtnContextBuf &buff ) ;
   INT32 _processMsg( const CHAR *pMsg ) ;
   INT32 _onMsgBegin( MsgHeader *msg ) ;
   INT32 _onMsgEnd( INT32 result, MsgHeader *msg ) ;
   INT32 _reply( MsgOpReply *replyHeader, const CHAR *pBody, const INT32 len ) ;
   INT32 _replyOpQuery( MsgOpReply *replyHeader, const CHAR *pBody, const INT32 len ) ;
   INT32 _replyOpMsg( MsgOpReply *replyHeader, const CHAR *pBody, const INT32 len ) ;

private:
   void  _resetBuffers() ;
   INT32 _setSeesionAttr() ;
   void  _handleResponse( const INT32 opType, engine::rtnContextBuf &buff ) ;
   void  _formatConversion ( INT32 opType, BSONObj &obj ) ;
   INT32 _autoCreateCL() ;
   INT32 _autoCreateCS() ;
   void  _buildFirstBatch( msgParser &parser, INT32 errCode,
                           engine::rtnContextBuf &replyBuf ) ;
   void  _buildNextBatch( msgParser &parser, INT32 errCode,
                          engine::rtnContextBuf &replyBuf ) ;

private:
   mongoConverter          _converter ;
   MsgOpReply              _replyHeader ;
   BOOLEAN                 _masterRead ;
   cursorStartFrom         _cursorStartFrom ;
   engine::rtnContextBuf   _contextBuff ;
   BSONObj                 _errorInfo ;
   msgBuffer               _inBuffer ;
   msgBuffer               _outBuffer ;
   msgBuffer               _tmpBuffer ;
   engine::IResource      *_resource ;
} ;

typedef _mongoSession mongoSession ;

#endif // _SDB_MONGO_MSG_CONVERTER_HPP_
