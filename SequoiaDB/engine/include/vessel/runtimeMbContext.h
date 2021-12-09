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

   Source File Name = runtimeMbContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_RUNTIME_MB_CONTEXT_H_
#define VESSEL_RUNTIME_MB_CONTEXT_H_

#include "vessel/strSlice.h"
#include "vessel/objectIdentifier.h"
#include "vessel/objectLatchMap.hpp"
#include "vessel/collectionRecordPage.h"

namespace engine
{
namespace vessel
{
   class runtimeMbContext : public SDBObject
   {
      public:
         runtimeMbContext(){}
         ~runtimeMbContext();
         runtimeMbContext(const runtimeMbContext &) = delete;
         runtimeMbContext &operator=(const runtimeMbContext &) = delete;

      public:
         void init(const collectionRecord &cmr,
                   const collectionSpaceId &csIdentifer);
         void fini();

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return _gcid.isValid();
         }
         OSS_INLINE const globalCollectionId &getGlobalId()const
         {
            return _gcid;
         }
         OSS_INLINE const strSlice &getName()const
         {
            return _name;
         }
         OSS_INLINE UINT32 getMinFreePercent()const
         {
            return _minFreePercent;
         }
         OSS_INLINE UTIL_COMPRESSOR_TYPE getCompressionType()const
         {
            return _compressionType;
         }
         OSS_INLINE RID_LATCH_CONTEXT &getRidLatchContext()
         {
            return _rlc;
         }
         OSS_INLINE const RID_LATCH_CONTEXT &getRidLatchContext()const
         {
            return _rlc;
         }

      private:
         globalCollectionId _gcid;
         strSlice _name;
         UINT8 _minFreePercent = 0;
         UTIL_COMPRESSOR_TYPE _compressionType = UTIL_COMPRESSOR_INVALID;

         RID_LATCH_CONTEXT _rlc;
   };//class runtimeMbContext
} // namespace vessel
  
} // namespace engine


#endif//VESSEL_RUNTIME_MB_CONTEXT_H_