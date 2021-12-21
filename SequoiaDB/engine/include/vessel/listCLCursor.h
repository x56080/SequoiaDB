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

   Source File Name = listCLCursor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LIST_CL_CURSOR_H_
#define VESSEL_LIST_CL_CURSOR_H_

#include "vessel/cursorKernal.h"
#include "ossMemPool.hpp"
#include "dms.hpp"
#include "vessel/objectIdentifier.h"

namespace engine
{
namespace vessel
{
   class listCLCursor : public cursorKernal
   {
      public:
         listCLCursor(){}
         virtual ~listCLCursor(){}

      public:
         virtual CURSOR_TYPE getType()const
         {
            return CURSOR_TYPE_LIST_COLLECTION;
         }

         OSS_INLINE void setCollectionSpace(const collectionSpaceId &id)
         {
            _id = id;
         }

         OSS_INLINE const collectionSpaceId &getIdentifier()const
         {
            return _id;
         }

         OSS_INLINE void setCLName(const CHAR *name)
         {
            ossStrcpy(_clName, name);
         }

         OSS_INLINE const CHAR *getCLName()const
         {
            return _clName;
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
         collectionSpaceId _id;
         CHAR _clName[DMS_COLLECTION_NAME_SZ + 1] = {};
         ossPoolSet<UINT32> _pushedLIds;
   };//class listCLCursor
}//namespace vessel
}//namespace engine

#endif//VESSEL_LIST_CL_CURSOR_H_