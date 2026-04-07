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

   Source File Name = sptUsrDiagLog.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/04/2026  JQQ Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_USR_DIAGLOG_HPP
#define SPT_USR_DIAGLOG_HPP
#include "sptApi.hpp"

namespace engine
{
   class _sptUsrDiagLog : public SDBObject
   {
      JS_DECLARE_CLASS( _sptUsrDiagLog )
   public:
      _sptUsrDiagLog() ;
      virtual ~_sptUsrDiagLog() ;
   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail ) ;
      INT32 destruct() ;
   } ;
   typedef _sptUsrDiagLog sptUsrDiagLog ;
}
#endif
