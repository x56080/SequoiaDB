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

#include "vessel/unstableIndexContext.h"
#include "vessel/vesselIdDef.h"
#include "vessel/shallowPointer.hpp"
#include "vessel/objectIdentifier.h"
#include "vessel/indexDescription.h"

namespace engine
{
namespace vessel
{
   class indexObject : public SDBObject
   {
      public:
         indexObject();
         ~indexObject();
         indexObject(const indexObject &) = delete;
         indexObject &operator=(const indexObject &) = delete;

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return _indexId.isValid();
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
         OSS_INLINE const indexIdentifier &getIndexId()const
         {
            return _indexId;
         }
         OSS_INLINE const indexDescription &getDescription()const
         {
            return _desc;
         }
         OSS_INLINE unstableIndexContext *getUnstatbleContext()const
         {
            return _unstatbleContext;
         }
         OSS_INLINE PAGE_ID getEntryLpid()const
         {
            return _entryLpid;
         }
         OSS_INLINE UINT32 getBtreeRootSplitTimes()const
         {
            return _btreeRootSplitTimes;
         }
         OSS_INLINE PAGE_ID getBtreeRoot()const
         {
            return _btreeRoot;
         }
      public:
         INT32 init(INT32 indexSlot,
                    UINT32 indexLid,
                    const indexDescription &desc,
                    INDEX_STATUS status,
                    PAGE_ID lpid,
                    PAGE_ID btreeRoot = INVALID_PAGE_ID);

         void fini();

         void dump(bson::BSONObjBuilder &builder)const;

         void removeUnstableContext();

         void updateStatus(INDEX_STATUS status);

         BOOLEAN associates(const CHAR *fieldName)const;

         void updateBtreeRoot(PAGE_ID root, UINT32 splitTimes);

         void updateBtreeRootSplitTimes(UINT32 splitTimes);

         BOOLEAN hasBtreeRoot()const;

         void removeBtreeRoot();

      private:
         indexIdentifier _indexId;
         indexDescription _desc;
         INDEX_STATUS _status = INDEX_STATUS_INVALID;
         PAGE_ID _entryLpid = INVALID_PAGE_ID;
         PAGE_ID _btreeRoot = INVALID_PAGE_ID;
         UINT32 _btreeRootSplitTimes = 0;
         unstableIndexContext *_unstatbleContext = NULL;
   };//class indexObject

   typedef shallowPointer<indexObject> INDEX_OBJECT_PTR;
} // namespace vessel

} // namespace engine


#endif//VESSEL_INDEX_OBJECT_H_