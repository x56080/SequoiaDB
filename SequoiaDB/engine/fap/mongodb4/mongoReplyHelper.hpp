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

   Source File Name = mongReplyHelper.hpp

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
#ifndef _SDB_MONGO_REPLY_HELPER_HPP_
#define _SDB_MONGO_REPLY_HELPER_HPP_

#include "msgBuffer.hpp"
#include "sdbInterface.hpp"
#include "rtnContextBuff.hpp"
#include "mongodef.hpp"

namespace fap
{
   enum FAP_WIRE_VERSION {
       FAP_MIN_WIRE_VERSION = 0,
       FAP_MAX_WIRE_VERSION = 8, // MongoDB(4.2+)
   };

   namespace mongo
   {
      void buildIsMasterReplyMsg( engine::rtnContextBuf &buff ) ;

      void buildGetLastErrorReplyMsg( const bson::BSONObj &err,
                                      engine::rtnContextBuf &buff ) ;

      void buildNotSupportReplyMsg( engine::rtnContextBuf &buff,
                                    const char *cmdName ) ;

      void buildPingReplyMsg( engine::rtnContextBuf &buff ) ;

      void buildGetMoreMsg( msgBuffer &out ) ;

      void buildWhatsmyuriReplyMsg( engine::rtnContextBuf &buff ) ;

      void buildGetLogReplyMsg( engine::rtnContextBuf &buff ) ;

      void buildBuildinfoReplyMsg( engine::rtnContextBuf &buff ) ;

      void buildAuthStep3ReplyMsg( engine::rtnContextBuf &buff ) ;

   }
}
#endif
