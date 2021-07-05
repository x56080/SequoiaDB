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

   Source File Name = lcBucketInnerIndex.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LC_BUCKET_INNER_INDEX_H_
#define VESSEL_LC_BUCKET_INNER_INDEX_H_

#include "ossMemPool.hpp"
#include "vessel/globalPageID.h"

namespace engine
{
namespace vessel
{
   class liteCachePageTag;
   typedef ossPoolMultiMap<GLOBAL_PAGE_ID, liteCachePageTag *> LC_BUCKET_INNER_INDEX;
   typedef LC_BUCKET_INNER_INDEX::iterator LC_BUCKET_INNER_INDEX_ITERATOR;
   typedef LC_BUCKET_INNER_INDEX::const_iterator LC_BUCKET_INNER_INDEX_CONST_ITERATOR;
}//namespace vessel
}//namespace engine

#endif//VESSEL_LC_BUCKET_INNER_INDEX_H_