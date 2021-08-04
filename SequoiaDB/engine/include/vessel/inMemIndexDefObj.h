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
#include "vessel/indexKeyPattern.h"
#include "vessel/memoryBlock.h"
#include "vessel/indexParameters.h"

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
         OSS_INLINE BOOLEAN isValid()const
         {
            return !_indexName.empty();
         }
         OSS_INLINE const strSlice &getIndexName()const
         {
            return _indexName;
         }
         OSS_INLINE const indexKeyPattern &getKeyPattern()const
         {
            return _keyPattern;
         }
         OSS_INLINE BOOLEAN isOwned()const
         {
            return isValid() && _defObj.data() == _mb.getBuffer();
         }

      public:

         void fini();

         INT32 init(const indexDefHead &head,
                    const slice &defObj);

         INT32 getOwned();

      private:
         INT32 _initFromDefObj(const slice &defObj);
      private:
         indexDefHead _head;
         strSlice _indexName;
         indexKeyPattern _keyPattern;
         indexParameters _params;
         slice _defObj;
         memoryBlock _mb;
   };//class inMemIndexDefObj
}//namespace vessel
}//namespace engine

#endif//VESSEL_IN_MEM_INDEX_DEF_OBJ_H_