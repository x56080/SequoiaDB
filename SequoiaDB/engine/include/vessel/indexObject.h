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

   Source File Name = indexObject.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_OBJECT_H_
#define VESSEL_INDEX_OBJECT_H_

#include "vessel/indexKeyPattern.h"
#include "vessel/indexDef.h"
#include "vessel/indexParameters.h"
#include "ossMemPool.hpp"

namespace engine
{
namespace vessel
{
   class indexObject : public SDBObject
   {
      public:
         indexObject();
         ~indexObject();

      public:
         INT32 init(INT32 indexSlot,
                    UINT32 indexId,
                    const strSlice &indexName,
                    const indexKeyPattern &pattern,
                    const indexParameters &params);

         void fini();

         OSS_INLINE BOOLEAN isValid()const
         {
            return INVALID_LOGICAL_INDEX_ID != _indexId;
         }


      private:
         INT32 _indexSlot = -1;
         UINT32 _indexId = INVALID_LOGICAL_INDEX_ID;
         ossPoolString _indexName;
         indexKeyPattern _pattern;
         indexParameters _params;
   };//class indexObject
}//namespace vessel
}//namesapce engine

#endif//VESSEL_INDEX_OBJECT_H_