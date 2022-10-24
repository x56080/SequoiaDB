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

   Source File Name = dpsTrivialStrField.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef DPS_TRIVIAL_STR_FIELD_HPP__
#define DPS_TRIVIAL_STR_FIELD_HPP__

#include "dpsTrivialStrDef.hpp"
#include "pdTrace.hpp"

namespace engine
{  
   class _dpsTrivialStrField : public SDBObject
   {
      friend class _dpsTrivialString;

      public:
         _dpsTrivialStrField() = default;

      private:
         explicit _dpsTrivialStrField(const void *data);
      
      public:
         OSS_INLINE BOOLEAN isValid() const
         {
            return nullptr != _fh;
         }

      public:
         DPS_TS_FIELD_TAG getTag() const;
         UINT32 getValueSize() const;
         UINT32 getFieldSize() const;
         const CHAR *getRawValue() const;
         BOOLEAN isEndingField() const;

         template<typename T>
         T getNumericValue() const
         {
            static_assert(std::numeric_limits<T>::is_specialized, "must be numeric");
            UINT32 size = getValueSize();
            SDB_ASSERT(size == sizeof(T), "must be same");
            return *reinterpret_cast<const T *>(getRawValue());
         }

      private:
         const dpsTsFieldHeader *_fh = nullptr;
   };//class _dpsTrivialStrField
   using dpsTrivialStrField = class _dpsTrivialStrField;
} // namespace engine


#endif//DPS_TRIVIAL_STR_FIELD_HPP__