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

   
*******************************************************************************/
#include <string.h>
#include "core.h"
#include "ossErr.h"
#include "sptParseMandoc.hpp"
SDB_EXTERN_C_START
#include "parseMandoc.h"
SDB_EXTERN_C_END

// class parseMandoc
_sptParseMandoc& _sptParseMandoc::getInstance()
{
   static _sptParseMandoc _instance ;
   return _instance ;
}

INT32 _sptParseMandoc::parse(const CHAR* filename)
{
   INT32 rc = SDB_OK ;
#if defined _WIN32
   const CHAR *argv[6] = { "sdb", "-K", "utf-8", "-T", "locale", filename } ;
#else
   const CHAR *argv[6] = { "sdb", "-K", "utf-8", "-T", "utf8", filename } ;
#endif
   rc = parse_mandoc(6, (const CHAR **)argv) ;
   if ( rc )
   {
      rc = SDB_SYS ;
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

