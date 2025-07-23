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

   Source File Name = clsRegAssit.hpp

   Descriptive Name = node register assistant

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   replication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who      Description
   ====== =========== ======== ==============================================
          10/10/2017  Ting YU  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CLSREGASSIT_HPP_
#define CLSREGASSIT_HPP_

#include "../bson/bsonobj.h"
#include "msg.hpp"
#include "ossSocket.hpp"

using namespace bson ;

namespace engine
{
   class _clsRegAssit : public SDBObject
   {
      public :
         _clsRegAssit () ;
         ~_clsRegAssit () ;
         INT32 buildRequestObj ( BSONObj &request ) ;
         INT32 extractResponseMsg ( MsgHeader *pMsg ) ;
         UINT32 getGroupID () ;
         UINT16 getNodeID () ;
         const CHAR* getHostname () ;

      private :
         UINT32 _groupID ;
         UINT16 _nodeID ;
         CHAR _hostName[ OSS_MAX_HOSTNAME + 1 ];
   } ;
   typedef _clsRegAssit clsRegAssit ;
}

#endif

