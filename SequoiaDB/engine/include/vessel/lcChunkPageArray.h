/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = lcChunkPageArray.h

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LC_CHUNK_PAGE_ARRAY_H_
#define VESSEL_LC_CHUNK_PAGE_ARRAY_H_

#include "vessel/lcChunkPage.h"

namespace engine
{
namespace vessel
{
   static const UINT32 LC_CP_ARRAY_DEFAULT_SIZE = 2;
   class lcChunkPageArray
   {
      public:
         OSS_INLINE lcChunkPageArray():
         _pages(_static),
         _size(LC_CP_ARRAY_DEFAULT_SIZE)
         {}

         ~lcChunkPageArray();

         OSS_INLINE BOOLEAN valid()const
         {
            return NULL != _pages;
         }

         OSS_INLINE lcChunkPage *pages()
         {
            return _pages;
         }

         OSS_INLINE UINT32 size() const
         {
            return _size;
         }

         INT32 resize(UINT32 size);
      private:
         lcChunkPage _static[LC_CP_ARRAY_DEFAULT_SIZE];
         lcChunkPage *_pages;
         UINT32 _size;
   }; /// end of class lcChunkPageArray
}
}
#endif

