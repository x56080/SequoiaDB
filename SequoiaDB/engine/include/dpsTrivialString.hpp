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

   Source File Name = dpsTrivialString.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef DPS_TRIVIAL_STRING_HPP__
#define DPS_TRIVIAL_STRING_HPP__

#include "dpsTrivialStrDef.hpp"
#include "utilSlice.hpp"
#include "dpsTrivialStrField.hpp"

namespace engine
{
   class _dpsTrivialString : public SDBObject
   {
      public:
         _dpsTrivialString() = default;
         explicit _dpsTrivialString(const CHAR *data, UINT32 size=0);

      public:
         OSS_INLINE void reset()
         {
            _size = 0;
            _data = nullptr;
            return;
         }
         
         OSS_INLINE BOOLEAN isValid()const {return nullptr != _data;}

         OSS_INLINE UINT32 getSize() const
         {
            return 0 == _size ? _calcAndCacheSize() : _size;
         }

      public:
         class iterator : public SDBObject
         {
            friend class _dpsTrivialString;
            public:
               iterator() = default;
            private:
               explicit iterator(const CHAR *);

            public:
               OSS_INLINE BOOLEAN isValid() const {return nullptr != _data;}
               OSS_INLINE void reset() {_data = nullptr;}
               dpsTrivialStrField getField() const;
               BOOLEAN next();

            private:
               const CHAR *_data = nullptr;
         };

         // dpsTrivialString::iterator itr = ts.begin();
         // while (itr.isValid())
         // {
         //    ...
         //    itr.next();
         // }
         iterator begin() const;

         dpsTrivialStrField seek(DPS_TS_FIELD_TAG tag) const;

      private:
         UINT32 _calcAndCacheSize() const;

      private:
         const CHAR *_data = nullptr;
         mutable UINT32 _size = 0;
   };//class _dpsTrivialString
   using dpsTrivialString = _dpsTrivialString;
} // namespace engine


#endif//DPS_TRIVIAL_STRING_HPP__
