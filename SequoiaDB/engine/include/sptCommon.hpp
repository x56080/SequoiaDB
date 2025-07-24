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

   Source File Name = sptCommon.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/28/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPTCOMMON_HPP__
#define SPTCOMMON_HPP__

#include "core.h"
#include "jsapi.h"

#define CMD_HELP           "help"
#define CMD_QUIT           "quit"
#define CMD_QUIT1          "quit;"
#define CMD_CLEAR          "clear"
#define CMD_CLEAR1         "clear;"
#define CMD_CLEARHISTORY   "history-c"
#define CMD_CLEARHISTORY1  "history-c;"

namespace engine
{
   /*
      Global function define
   */
   const CHAR *sdbGetErrMsg() ;
   void  sdbSetErrMsg( const CHAR *err ) ;
   BOOLEAN sdbIsErrMsgEmpty() ;

   INT32 sdbGetErrno() ;
   void  sdbSetErrno( INT32 errNum ) ;

   // clear msg and errno
   void  sdbClearErrorInfo() ;

   BOOLEAN  sdbNeedPrintError() ;
   void     sdbSetPrintError( BOOLEAN print ) ;

   void     sdbSetReadData( BOOLEAN hasRead ) ;
   BOOLEAN  sdbHasReadData() ;

   void     sdbSetNeedClearErrorInfo( BOOLEAN need ) ;
   BOOLEAN  sdbIsNeedClearErrorInfo() ;

   void     sdbReportError( JSContext *cx, const char *msg,
                            JSErrorReport *report ) ;

   void     sdbReportError( const CHAR *filename, UINT32 lineno,
                            const CHAR *msg, BOOLEAN isException ) ;

}

#endif //SPTCOMMON_HPP__

