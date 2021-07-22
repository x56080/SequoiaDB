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

   Source File Name = bitmapVector.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BITMAP_VECTOR_H_
#define VESSEL_BITMAP_VECTOR_H_

#include "ossMemPool.hpp"

namespace engine
{
namespace vessel
{
   class bitmapVector : public SDBObject
   {
      public:
         bitmapVector();
         ~bitmapVector();

         bitmapVector(const bitmapVector &) = delete;
         bitmapVector &operator=(const bitmapVector &) = delete;

      public:
         OSS_INLINE BOOLEAN isReady()const
         {
            return 0 < _bitsCapacity;
         }

      public:
         OSS_INLINE UINT32 getBitsCapcity()const {return _bitsCapacity;}
         INT32 init(UINT32 bitsCapacity);
         void fini();
         INT32 setNoFree(UINT32 offset, BOOLEAN &alreadyNoFree);
         INT32 extendBitmapVec(UINT32 size);

      private:
         typedef ossPoolVector<UINT64 *> _BITMAP_VEC;
      private:
         UINT32 _bitsCapacity = 0;
         _BITMAP_VEC _bitmapVec;
         
   };//class bitmapVector
}//namespace vessel
}//namespace engine

#endif//VESSEL_BITMAP_VECTOR_H_