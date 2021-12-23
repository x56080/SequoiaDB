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

   Source File Name = lcBucket.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LC_BUCKET_H_
#define VESSEL_LC_BUCKET_H_

#include "vessel/globalPageID.h"
#include "vessel/lcPageTagHolder.h"
#include "ossMemPool.hpp"
#include "vessel/mmapPagePointer.h"
#include "vessel/lcBucketInnerIndex.h"
#include "ossMemPool.hpp"

namespace engine
{
namespace vessel
{
   class storageUnit;
   
   class lcBucket : public SDBObject
   {
      public:
         lcBucket();
         ~lcBucket();
         lcBucket(const lcBucket &) = delete;
         lcBucket &operator=(const lcBucket &) = delete;

      public:
         INT32 ensureTagAndIncUsage(const GLOBAL_PAGE_ID &id,
                                    const mmapPagePointer &ptr,
                                    lcPageTagHolder &holder,
                                    BOOLEAN &isNewTag);

         BOOLEAN getTagAndIncUsage(const GLOBAL_PAGE_ID &id,
                                   lcPageTagHolder &holder);

         void discardAndPinTags(SPACE_ID sid,
                                ossPoolList<liteCachePageTag *> &tags);
      private:                           
         INT32 insertTag(const GLOBAL_PAGE_ID &id,
                         const mmapPagePointer &ptr,
                         lcPageTagHolder &holder);

         liteCachePageTag *recycleTag();

      private:
         void pushFront(liteCachePageTag *tag);

         void pushBack(liteCachePageTag *tag);

         void removeFromList(liteCachePageTag *tag);

         liteCachePageTag *popBack();
                                                
      private:
         LC_BUCKET_INNER_INDEX _tagIndex;
         liteCachePageTag *_head = NULL;
         liteCachePageTag *_tail = NULL;
   };
} /// end of namespace vessel
} /// end of namespace engine

#endif