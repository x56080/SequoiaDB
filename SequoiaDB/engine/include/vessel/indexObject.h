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
#include "vessel/indexDescription.h"
#include "ossMemPool.hpp"
#include "vessel/indexEntryPage.h"
#include "vessel/objectIdentifier.h"

namespace engine
{
namespace vessel
{
   class indexObject : public SDBObject
   {
      public:
         indexObject(){}
         ~indexObject(){}
         indexObject(const indexObject &) = delete;
         indexObject &operator=(const indexObject &o)
         {
            _indexId = o._indexId;
            _desc = o._desc;
            _btreeRootSplitTimes = o._btreeRootSplitTimes;
            _btreeRoot = o._btreeRoot;
            return *this;
         }

      public:
         INT32 init(INT32 indexSlot,
                    UINT32 indexLid,
                    const indexDescription &desc,
                    PAGE_ID btreeRoot=INVALID_PAGE_ID);

         void fini();

         OSS_INLINE BOOLEAN isValid()const
         {
            return _indexId.isValid() &&
                   _desc.isValid();
         }
         OSS_INLINE UINT32 getLogicalIndexId()const
         {
            return _indexId.getLogicalIndexId();
         }
         OSS_INLINE const strSlice &getIndexName()const
         {
            return _desc.getNameSlice();
         }
         OSS_INLINE const indexKeyPattern &getPattern()const
         {
            return _desc.getPattern();
         }
         OSS_INLINE INDEX_TYPE getIndexType()const
         {
            return _desc.getType();
         }
         OSS_INLINE const indexDescription &getDescription()const
         {
            return _desc;
         }
         OSS_INLINE UINT32 getBtreeRootSplitTimes()const
         {
            return _btreeRootSplitTimes;
         }
         OSS_INLINE PAGE_ID getBtreeRoot()const
         {
            return _btreeRoot;
         }
         void updateBtreeRoot(PAGE_ID root, UINT32 splitTimes);

         void updateBtreeRootSplitTimes(UINT32 splitTimes);

         BOOLEAN hasBtreeRoot()const;

         void removeBtreeRoot();

      private:
         indexIdentifier _indexId;
         indexDescription _desc;
         UINT32 _btreeRootSplitTimes = 0;
         PAGE_ID _btreeRoot = INVALID_PAGE_ID;
   };//class indexObject
}//namespace vessel
}//namesapce engine

#endif//VESSEL_INDEX_OBJECT_H_