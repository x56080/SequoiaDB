/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = dpsTrivialStrField.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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