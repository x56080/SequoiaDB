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

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

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

      public:
         INT32 ensureTagAndIncUsage(const GLOBAL_PAGE_ID &id,
                                    UINT32 pageSize,
                                    UINT32 minRecycleCount,
                                    lcPageTagHolder &holder,
                                    BOOLEAN &newTagInBucket);

         BOOLEAN getTagAndIncUsage(const GLOBAL_PAGE_ID &id,
                                   lcPageTagHolder &holder);

         INT32 releaseRemovedTag(liteCachePageTag *tag);
      private:                           
         INT32 insertTag(const GLOBAL_PAGE_ID &id,
                         UINT32 pageSize,
                         UINT32 minRecycleCount,
                         lcPageTagHolder &holder);

         liteCachePageTag *recycleTag(UINT32 minRecycleCount);

      private:
         typedef ossPoolMultiMap<GLOBAL_PAGE_ID, liteCachePageTag*> _TAG_MAP;
         typedef _TAG_MAP::iterator _TAG_MAP_ITERATOR;
         typedef _TAG_MAP::const_iterator _TAG_MAP_CONST_ITERATOR;
                                                
      private:
         _TAG_MAP _tags;
   };
} /// end of namespace vessel
} /// end of namespace engine

#endif