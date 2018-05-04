/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = sptRemote.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/18/2016  WJM Initial Draft

   Last Changed =

*******************************************************************************/


#ifndef SPT_REMOTE_HPP__
#define SPT_REMOTE_HPP__

#include "msg.h"
#include "oss.hpp"


namespace engine
{

   /*
      sptRemote define
   */
   class _sptRemote : public SDBObject
   {
      public:
         _sptRemote() ;

         ~_sptRemote() ;

      public:

         INT32       runCommand( ossValuePtr handle,
                                 const CHAR *pString,
                                 SINT32 flag,
                                 UINT64 reqID,
                                 SINT64 numToSkip,
                                 SINT64 numToReturn,
                                 const CHAR *arg1,
                                 const CHAR *arg2,
                                 const CHAR *arg3,
                                 const CHAR *arg4,
                                 CHAR **ppRetBuffer,
                                 INT32 &retCode,
                                 BOOLEAN needRecv = TRUE ) ;

      private:
         INT32       _sendAndRecv( ossValuePtr handle,
                                   const MsgHeader *sendMsg,
                                   MsgHeader **recvMsg,
                                   INT32 *size,
                                   BOOLEAN needRecv,
                                   BOOLEAN endianConvert ) ;

         INT32       _sendMsg( ossValuePtr handle, const MsgHeader *msg,
                               BOOLEAN endianConvert ) ;

         INT32       _recvMsg( ossValuePtr handle,
                               MsgHeader **msg,
                               INT32 *msgLength,
                               BOOLEAN endianConvert ) ;

         INT32       _send( ossValuePtr handle,
                            const CHAR *pMsg,
                            INT32 msgLength ) ;

         INT32       _reallocBuffer( CHAR **ppBuffer, INT32 *bufferSize,
                                     INT32 newSize ) ;

         INT32       _extract( MsgHeader *msg, INT32 msgSize,
                               SINT64 *contextID,
                               BOOLEAN &extracted,
                               BOOLEAN endianConvert ) ;

         INT32       _getRetBuffer( CHAR *pRetMsg, CHAR **ppRetBuffer ) ;

   } ;
   typedef _sptRemote sptRemote ;
}
#endif