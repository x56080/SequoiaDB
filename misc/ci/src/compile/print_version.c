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

   Source File Name = print_version.c

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include <stdio.h>
#include "ossVer.h"
int main()
{
	 /*
	 * this file just to print the software version on the screen for ant to catch 
	 */
#ifdef SDB_ENGINE_FIXVERSION_CURRENT
   printf("%d.%d.%d",SDB_ENGINE_VERISON_CURRENT , SDB_ENGINE_SUBVERSION_CURRENT,
          SDB_ENGINE_FIXVERSION_CURRENT );
#else
   printf("%d.%d",SDB_ENGINE_VERISON_CURRENT , SDB_ENGINE_SUBVERSION_CURRENT );
#endif // SDB_ENGINE_FIXVERSION_CURRENT
   return 0;
}