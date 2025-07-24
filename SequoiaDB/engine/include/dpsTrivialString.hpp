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

   Source File Name = dpsTrivialString.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
