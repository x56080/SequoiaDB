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
#include "vessel/indexEntryPage.h"

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
         indexObject &operator=(const indexObject &) = delete;

      public:
         INT32 shallowInit(UINT32 indexId,
                           const strSlice &indexName,
                           const indexKeyPattern &pattern,
                           const indexParameters &params,
                           PAGE_ID btreeRoot=INVALID_PAGE_ID);

         void shallowCopy(const indexObject &o);

         void fini();

         void getOwned();

         BOOLEAN isOwned()const;

         OSS_INLINE BOOLEAN isValid()const
         {
            return INVALID_LOGICAL_INDEX_ID != _indexId;
         }
         OSS_INLINE UINT32 getIndexID()const
         {
            return _indexId;
         }
         OSS_INLINE const strSlice &getIndexName()const
         {
            return _nameSlice;
         }
         OSS_INLINE const indexParameters &getParams()const
         {
            return _params;
         }
         OSS_INLINE const indexKeyPattern &getPattern()const
         {
            return _pattern;
         }
         OSS_INLINE INDEX_TYPE getIndexType()const
         {
            return _params.type;
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

      private:
         UINT32 _indexId = INVALID_LOGICAL_INDEX_ID;
         strSlice _nameSlice;
         ossPoolString _indexName;
         indexKeyPattern _pattern;
         indexParameters _params;
         UINT32 _btreeRootSplitTimes = 0;
         PAGE_ID _btreeRoot = INVALID_PAGE_ID;
   };//class indexObject
}//namespace vessel
}//namesapce engine

#endif//VESSEL_INDEX_OBJECT_H_