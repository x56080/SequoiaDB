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

   Source File Name = sptUsrDiagLog.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/04/2026  JQQ Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptUsrDiagLog.hpp"

using namespace bson ;
namespace engine
{
   #define SPT_DIAGLOG_NAME         "DiagLog"

   JS_CONSTRUCT_FUNC_DEFINE( _sptUsrDiagLog, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptUsrDiagLog, destruct )

   JS_BEGIN_MAPPING( _sptUsrDiagLog, SPT_DIAGLOG_NAME )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
   JS_MAPPING_END()

   _sptUsrDiagLog::_sptUsrDiagLog()
   {
   }

   _sptUsrDiagLog::~_sptUsrDiagLog()
   {
   }

   INT32 _sptUsrDiagLog::construct( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail )
   {
      return SDB_OK ;
   }

   INT32 _sptUsrDiagLog::destruct()
   {
      return SDB_OK ;
   }
}
