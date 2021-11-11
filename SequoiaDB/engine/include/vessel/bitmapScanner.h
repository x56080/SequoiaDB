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

   Source File Name = bitmapScanner.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BITMAP_SCANNER_H_
#define VESSEL_BITMAP_SCANNER_H_

#include "core.hpp"
#include "oss.hpp"

namespace engine
{
namespace vessel
{
   class bitmapScanner : public SDBObject
   {
      public:
         bitmapScanner(){}
         ~bitmapScanner(){}
         bitmapScanner(const bitmapScanner &) = delete;
         bitmapScanner &operator=(const bitmapScanner &) = delete;
      public:
         void init(UINT64 *bitmap,
                   UINT32 capacity,
                   BOOLEAN zeroed);

         void load(UINT64 *bitmap,
                   UINT32 capacity);

         void reset();

         OSS_INLINE BOOLEAN isReady()const
         {
            return NULL != _bitmap;
         }

         BOOLEAN findAndClearNext(INT32 &offset);

         BOOLEAN moveToNextUnzeroPos(INT32 &offset);

         /// set offset as nonzero
         void setBit(UINT32 offset, BOOLEAN resetPosToBackward=FALSE);

         BOOLEAN clearBit(UINT32 offset);

         BOOLEAN testBit(UINT32 offset)const;

         OSS_INLINE BOOLEAN allZeroed()const
         {
            return 0 == _nonzeroed;
         }

         OSS_INLINE UINT32 getNonZeroedCount()const
         {
            return _nonzeroed;
         }

         OSS_INLINE UINT32 getCapacity()const
         {
            return _size << 6;
         }
      private:
         UINT64 *_bitmap = NULL;
         UINT32 _size = 0;
         UINT32 _nonzeroed = 0;
         UINT32 _pos = 0;
   };//class bitmapScanner
} // namespace vessel

} // namespace engine


#endif//VESSEL_BITMAP_SCANNER_H_