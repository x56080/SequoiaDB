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

   Source File Name = indexContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_CONTEXT_H_
#define VESSEL_INDEX_CONTEXT_H_

#include "vessel/indexObject.h"
#include "vessel/unstableIndexContext.h"
#include "vessel/vesselIdDef.h"

namespace engine
{
namespace vessel
{
   class indexContext : public SDBObject
   {
      public:
         indexContext();
         ~indexContext();
         indexContext(const indexContext &) = delete;
         indexContext &operator=(const indexContext &) = delete;

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return isValidIndexSlot(_indexSlot);
         }
         OSS_INLINE INT32 getIndexSlot()const
         {
            return _indexSlot;
         }
         OSS_INLINE BOOLEAN isNormal()const
         {
            return INDEX_STATUS_NORMAL == _status;
         }
         OSS_INLINE BOOLEAN isBuilding()const
         {
            return INDEX_STATUS_BUILDING == _status;
         }
         OSS_INLINE BOOLEAN isTruncating()const
         {
            return INDEX_STATUS_TRUNCATING == _status;
         }
         OSS_INLINE BOOLEAN isRemoving()const
         {
            return INDEX_STATUS_REMOVING == _status;
         }
         OSS_INLINE INDEX_STATUS getStatus()const
         {
            return _status;
         }
         OSS_INLINE const indexObject &getObj()const
         {
            return _obj;
         }
         OSS_INLINE UINT32 getIndexID()const
         {
            return _obj.getIndexID();
         }
         OSS_INLINE unstableIndexContext *getUnstatbleContext()const
         {
            return _unstatbleContext;
         }

         OSS_INLINE PAGE_ID getEntryLpid()const
         {
            return _lpid;
         }
      public:
         INT32 init(INT32 indexSlot,
                    PAGE_ID lpid,
                    const indexObject &obj,
                    INDEX_STATUS status);

         void fini();

         void dump(bson::BSONObjBuilder &builder)const;

         void setNormalFromBuilding();

         void setRemoving();

      private:
         INT32 _indexSlot = -1;
         PAGE_ID _lpid = INVALID_PAGE_ID;
         indexObject _obj;
         INDEX_STATUS _status = INDEX_STATUS_INVALID;
         unstableIndexContext *_unstatbleContext = NULL;

   };//class indexContext
} // namespace vessel

} // namespace engine


#endif//VESSEL_INDEX_CONTEXT_H_