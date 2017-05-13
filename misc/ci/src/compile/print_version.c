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