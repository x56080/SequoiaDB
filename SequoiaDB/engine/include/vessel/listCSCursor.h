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

   Source File Name = listCSCursor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LIST_CS_CURSOR_H_
#define VESSEL_LIST_CS_CURSOR_H_

#include "vessel/cursorKernal.h"
#include "ossMemPool.hpp"
#include "dms.hpp"
#include "interface/IRecordFilter.h"

namespace engine
{
namespace vessel
{
   class listCSCursor : public cursorKernal
   {
      public:
         listCSCursor(){}
         virtual ~listCSCursor(){}

      public:
         virtual CURSOR_TYPE getType()const
         {
            return CURSOR_TYPE_LIST_COLLECTION_SPACE;
         }

         OSS_INLINE void setLastName(const CHAR *name)
         {
            ossStrcpy(_csName, name);
         }

         OSS_INLINE const CHAR *getCSName()const
         {
            return _csName;
         }

         OSS_INLINE void markLIdPushed(UINT32 lid)
         {
            _pushedLIds.insert(lid);
         }

         OSS_INLINE BOOLEAN isPushed(UINT32 lid)const
         {
            return 0 < _pushedLIds.count(lid);
         }

      private:
         CHAR _csName[DMS_COLLECTION_SPACE_NAME_SZ + 1] = {};
         ossPoolSet<UINT32> _pushedLIds;
   };//class listCSCursor
}//namespace vessel
}//namespace engine

#endif//VESSEL_LIST_CS_CURSOR_H_