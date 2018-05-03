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

   Source File Name = omagentHelper.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/30/2014  TZB Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OMAGENT_HELPER_HPP_
#define OMAGENT_HELPER_HPP_

#include "core.hpp"
#include "pd.hpp"
#include "ossUtil.hpp"
#include "ossTypes.hpp"
#include "../bson/bson.hpp"
#include "omagentSyncCmd.hpp"
#include "omagentBackgroundCmd.hpp"

using namespace bson ;

namespace engine
{
   BOOLEAN omaIsCommand ( const CHAR *name ) ;

   INT32 omaParseCommand ( const CHAR *name,
                           _omaCommand **ppCommand ) ;

   INT32 omaInitCommand ( _omaCommand *pCommand ,INT32 flags,
                          INT64 numToSkip,
                          INT64 numToReturn, const CHAR *pMatcherBuff,
                          const CHAR *pSelectBuff, const CHAR *pOrderByBuff,
                          const CHAR *pHintBuff ) ;

   INT32 omaRunCommand ( _omaCommand *pCommand, CHAR **ppBody,
                         INT32 &bodyLen ) ;

   INT32 omaRunCommand ( _omaCommand *pCommand, BSONObj &result ) ;

   INT32 omaReleaseCommand ( _omaCommand **ppCommand ) ;

   // build reply buffer
   INT32 omaBuildReplyMsgBody ( CHAR **ppBuffer, INT32 *bufferSize,
                                SINT32 numReturned,
                                vector<BSONObj> *objList ) ;
   INT32 omaBuildReplyMsgBody ( CHAR **ppBuffer, INT32 *bufferSize,
                                SINT32 numReturned,
                                const BSONObj *bsonobj ) ;

}

#endif // OMAGENT_HELPER_HPP_

