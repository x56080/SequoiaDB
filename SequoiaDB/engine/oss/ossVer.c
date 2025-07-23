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

   Source File Name = ossVer.cpp

   Descriptive Name = Operating System Services Version

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains engine version information

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/16/2013  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossVer.h"
#include "ossUtil.h"
#include <iostream>

void ossGetVersion ( INT32 *version,
                     INT32 *subVersion,
                     INT32 *fixVersion,
                     INT32 *release,
                     const CHAR **ppBuild )
{
   if ( version )
      *version = SDB_ENGINE_VERISON_CURRENT ;
   if ( subVersion )
      *subVersion = SDB_ENGINE_SUBVERSION_CURRENT ;
   if ( release )
      *release = SDB_ENGINE_RELEASE_CURRENT ;
   if ( fixVersion )
   {
#ifdef SDB_ENGINE_FIXVERSION_CURRENT
      *fixVersion = SDB_ENGINE_FIXVERSION_CURRENT ;
#else
      *fixVersion = 0 ;
#endif // SDB_ENGINE_FIXVERSION_CURRENT
   }
   if ( ppBuild )
      *ppBuild = SDB_ENGINE_BUILD_TIME ;
}

void ossGetSimpleVersion( CHAR *pBuff, UINT32 bufLen )
{
   if ( NULL == pBuff || 0 == bufLen )
   {
      return ;
   }
   ossMemset( pBuff, 0, bufLen ) ;

#ifdef SDB_ENGINE_FIXVERSION_CURRENT
   ossSnprintf( pBuff, bufLen - 1, "%d.%d.%d",
                SDB_ENGINE_VERISON_CURRENT,
                SDB_ENGINE_SUBVERSION_CURRENT,
                SDB_ENGINE_FIXVERSION_CURRENT ) ;
#else
   ossSnprintf( pBuff, bufLen - 1, "%d.%d",
                SDB_ENGINE_VERISON_CURRENT,
                SDB_ENGINE_SUBVERSION_CURRENT ) ;
#endif
}

void ossSprintVersion( const CHAR *prompt, CHAR *pBuff, UINT32 len,
                       BOOLEAN multiLine )
{
   if ( 0 == len )
   {
      return ;
   }
   ossMemset( pBuff, 0, len ) ;

#ifdef SDB_ENGINE_FIXVERSION_CURRENT
   if ( multiLine )
   {
      ossSnprintf( pBuff, len - 1, "%s: %d.%d.%d%sRelease: %d%s%s%s",
                   prompt, SDB_ENGINE_VERISON_CURRENT,
                   SDB_ENGINE_SUBVERSION_CURRENT,
                   SDB_ENGINE_FIXVERSION_CURRENT,
                   OSS_NEWLINE, SDB_ENGINE_RELEASE_CURRENT,
                   OSS_NEWLINE, SDB_ENGINE_BUILD_TIME,
                   OSS_NEWLINE ) ;
   }
   else
   {
      ossSnprintf( pBuff, len - 1, "%s: %d.%d.%d, Release: %d, Build: %s",
                   prompt, SDB_ENGINE_VERISON_CURRENT,
                   SDB_ENGINE_SUBVERSION_CURRENT,
                   SDB_ENGINE_FIXVERSION_CURRENT,
                   SDB_ENGINE_RELEASE_CURRENT,
                   SDB_ENGINE_BUILD_TIME ) ;
   }
#else
   if ( multiLine )
   {
      ossSnprintf( pBuff, len - 1, "%s: %d.%d%sRelease: %d%s%s%s",
                   prompt, SDB_ENGINE_VERISON_CURRENT,
                   SDB_ENGINE_SUBVERSION_CURRENT,
                   OSS_NEWLINE, SDB_ENGINE_RELEASE_CURRENT,
                   OSS_NEWLINE, SDB_ENGINE_BUILD_TIME,
                   OSS_NEWLINE ) ;
   }
   else
   {
      ossSnprintf( pBuff, len - 1, "%s: %d.%d, Release: %d, Build: %s",
                   prompt, SDB_ENGINE_VERISON_CURRENT,
                   SDB_ENGINE_SUBVERSION_CURRENT,
                   SDB_ENGINE_RELEASE_CURRENT,
                   SDB_ENGINE_BUILD_TIME ) ;
   }
#endif //SDB_ENGINE_FIXVERSION_CURRENT
}

void ossPrintVersion( const CHAR *prompt )
{
   CHAR verText[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
   ossSprintVersion( prompt, verText, sizeof( verText ), TRUE ) ;
   std::cout << verText ;
}

