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

   Source File Name = utilCBsonMemAssit.cpp

   Descriptive Name = Data Protection Services Types Header

   When/how to use: this program may be used on binary and text-formatted
   versions of dps component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          29/05/2019  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilMemListPool.hpp"
#include "../client/bson/bson.h"

namespace engine
{

   class _utilCBsonMemAssist
   {
      public:
         _utilCBsonMemAssist()
         {
            bson_set_malloc_func( (bson_malloc_func_p)utilTCAlloc ) ;
            bson_set_realloc_func( (bson_realloc_func_p)utilTCRealloc ) ;
            bson_set_free_func( (bson_free_func_p)utilTCRelease ) ;
         }
   } ;

   _utilCBsonMemAssist s_tmpAssist ;

}

