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

   Source File Name = ossLatch.cpp

   Descriptive Name = Operating System Services Latch

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains functions for latch operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossLatch.hpp"

void ossLatch ( ossSLatch *latch, OSS_LATCH_MODE mode )
{
   if ( SHARED == mode )
      latch->get_shared () ;
   else
      latch->get () ;
}
void ossLatch ( ossXLatch *latch )
{
   latch->get () ;
}
void ossUnlatch ( ossSLatch *latch, OSS_LATCH_MODE mode )
{
   if ( SHARED == mode )
      latch->release_shared () ;
   else
      latch->release();
}
void ossUnlatch ( ossXLatch *latch )
{
   latch->release () ;
}
BOOLEAN ossTestAndLatch ( ossSLatch *latch, OSS_LATCH_MODE mode )
{
   if ( SHARED == mode )
      return latch->try_get_shared () ;
   else
      return latch->try_get();
}
BOOLEAN ossTestAndLatch ( ossXLatch *latch )
{
   return latch->try_get () ;
}

