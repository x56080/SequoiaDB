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

   Source File Name = requestBatch.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_REQUEST_BATCH_H_
#define VESSEL_REQUEST_BATCH_H_

#include "vessel/slice.h"
#include "utilArray.hpp"
#include "utilResult.hpp"
#include "../bson/bson.hpp"
#include "vessel/vesselIdDef.h"

namespace engine
{
namespace vessel
{
   class requestBatch : public SDBObject
   {
      public:
         requestBatch(){}
         ~requestBatch(){}
         requestBatch(const requestBatch &) = delete;
         requestBatch &operator=(const requestBatch &) = delete;

      private:
         typedef std::pair<STRIPING_ID, slice> _DATA;
         typedef _utilArray<std::pair<STRIPING_ID, slice>, 8> _BATCH; 

      public:
         OSS_INLINE void reset()
         {
            _batch.clear();
         }
         
         OSS_INLINE UINT32 getSize()const
         {
            return _batch.size();
         }
         OSS_INLINE BOOLEAN isEmpty()const
         {
            return 0 == _batch.size();
         }

         slice get(UINT32 pos, STRIPING_ID &striping)const;

         INT32 add(STRIPING_ID striping, const slice &data);

         INT32 addObjs(UINT32 count, const BSONObj &objs);
      private:
         _BATCH _batch;
   };//class requestBatch
} // namespace vessel

} // namespace engine


#endif//VESSEL_INSERT_BATCH_H_