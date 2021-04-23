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

   Source File Name = inMemIndexDefObj.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_IN_MEM_INDEX_DEF_OBJ_H_
#define VESSEL_IN_MEM_INDEX_DEF_OBJ_H_

#include "vessel/indexDefPage.h"
#include "ossMemPool.hpp"
#include "vessel/strSlice.h"
#include "vessel/indexOptions.h"
#include "vessel/indexKeyPattern.h"

namespace engine
{
namespace vessel
{

   class inMemIndexDefObj : public SDBObject
   {
      public:
         inMemIndexDefObj();
         ~inMemIndexDefObj();
         inMemIndexDefObj(const inMemIndexDefObj &) = delete;
         inMemIndexDefObj &operator=(const inMemIndexDefObj &) = delete;

      public:
         /// estimate size needed on disk.
         static UINT32 estimate(const strSlice &indexName,
                                const indexKeyPattern &keyPattern);

         void reset();
      private:
         indexDefRecord _record;
         strSlice _indexName;
         indexKeyPattern _keyPattern;
         UINT32 _bufferSize = 0;
         CHAR *_buffer = NULL;
   };//class inMemIndexDefObj
}//namespace vessel
}//namespace engine

#endif//VESSEL_IN_MEM_INDEX_DEF_OBJ_H_