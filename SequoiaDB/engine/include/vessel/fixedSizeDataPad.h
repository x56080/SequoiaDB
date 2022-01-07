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

   Source File Name = fixedSizeDataPad.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_FIXED_SIZE_DATA_PAD_H_
#define VESSEL_FIXED_SIZE_DATA_PAD_H_

#include "vessel/slice.h"
#include <initializer_list>

namespace engine
{
namespace vessel
{
   class fixedSizeDataPad : public SDBObject
   {
      public:
         fixedSizeDataPad(){}
         ~fixedSizeDataPad(){}
         fixedSizeDataPad(fixedSizeDataPad &&o);
         fixedSizeDataPad &operator=(fixedSizeDataPad &&o);
         fixedSizeDataPad(const fixedSizeDataPad &) = delete;
         fixedSizeDataPad &operator=(const fixedSizeDataPad &) = delete;

      public:
         OSS_INLINE BOOLEAN isValid()const {return NULL != _buffer;}
         void init(UINT32 bufferSize, CHAR *buffer);
         void clear();
         void fini();
         INT32 push(const slice &row);
         INT32 pushRowFragments(std::initializer_list<slice> il);
         BOOLEAN isFreeToPush(UINT32 size)const;
         slice getRow(UINT32 pos)const;
         UINT32 getRowSize(UINT32 pos)const;
         INT32 overwrite(const fixedSizeDataPad &pad);
         OSS_INLINE UINT32 getCount()const {return _count;}
         OSS_INLINE UINT32 getBufferSize()const {return _bufferSize;}
         OSS_INLINE UINT32 getFreeSize()const
         {
            return _backOffset - getFrontOffset();
         }
         OSS_INLINE UINT32 getUnfreeSize()const
         {
            return _bufferSize - getFreeSize();
         }

         static UINT32 getSavingSize(UINT32 size);

      private:
         struct _tag : public SDBObject
         {
            _tag(){}
            ~_tag(){}
            explicit _tag(UINT32 o, UINT32 s):
            offset(o),
            size(s){}
            _tag(const _tag &o):
            offset(o.offset),
            size(o.size){}
            _tag &operator=(const _tag &o)
            {
               offset = o.offset;
               size = o.size;
               return *this;
            }

            UINT32 offset = 0;
            UINT32 size = 0;
         };//struct _tag

      private:
         OSS_INLINE UINT32 getFrontOffset()const
         {
            return _count * sizeof(_tag);
         }

         const _tag *getTag(UINT32 pos)const;

      private:
         UINT32 _bufferSize = 0;
         CHAR *_buffer = NULL;
         UINT32 _count = 0;
         UINT32 _backOffset = 0;
   };//class fixedSizeDataPad
} // namespace vessel

} // namespace engine


#endif//VESSEL_FIXED_SIZE_DATA_PAD_H_