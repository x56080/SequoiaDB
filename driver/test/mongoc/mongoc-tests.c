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

   Source File Name = mongoc-tests.c

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "mongoc-tests.h"

char *TEST_RESULT;

void
run_test (const char *name,
          void (*func) (void))
{
   struct timeval begin;
   struct timeval end;
   struct timeval diff;
   double format;

   TEST_RESULT = "PASS";

   fprintf(stdout, "%-42s : ", name);
   fflush(stdout);
   bson_gettimeofday(&begin);
   func();
   bson_gettimeofday(&end);
   fprintf(stdout, "%s", TEST_RESULT);

   diff.tv_sec = end.tv_sec - begin.tv_sec;
   diff.tv_usec = end.tv_usec - begin.tv_usec;

   if (diff.tv_usec < 0) {
      diff.tv_sec -= 1;
      diff.tv_usec = diff.tv_usec + 1000000;
   }

   format = diff.tv_sec + (diff.tv_usec / 1000000.0);
   fprintf(stdout, " : %lf\n", format);
}
