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

   Source File Name = dpsPageMeta.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DPSPAGEMETA_HPP_
#define DPSPAGEMETA_HPP_

#include "core.hpp"

namespace engine
{
   struct _dpsPageMeta
   {
      INT32 offset ;
      INT32 beginSub ;
      UINT32 pageNum ;
      UINT32 totalLen ;

      _dpsPageMeta()
      :offset(-1),
       beginSub(-1),
       pageNum(0),
       totalLen(0)
      {

      }

      BOOLEAN valid()const
      {
         return -1 != offset &&
                -1 != beginSub &&
                0 != pageNum &&
                0 != totalLen ;
      }

      void clear()
      {
         if ( -1 != offset )
         {
            offset = -1 ;
            beginSub = -1 ;
            pageNum = 0 ;
            totalLen = 0 ;
         }
         SDB_ASSERT( -1 == beginSub &&
                     0 == pageNum &&
                     0 == totalLen, "impossible" ) ;
      }
   } ;
   typedef struct _dpsPageMeta dpsPageMeta ;
}

#endif

