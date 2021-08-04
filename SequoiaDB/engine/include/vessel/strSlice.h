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

   Source File Name = strSlice.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_STR_SLICE_H_
#define VESSEL_STR_SLICE_H_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.h"

namespace engine
{
namespace vessel
{
   class strSlice : public SDBObject
   {
      public:
         OSS_INLINE strSlice():
         _strLen(0),
         _str(NULL)
         {}

         OSS_INLINE strSlice(const strSlice &r):
         _strLen(r._strLen),
         _str(r._str)
         {}

         OSS_INLINE explicit strSlice(const CHAR *str, UINT32 len):
         _strLen(len),
         _str(str)
         {}

         OSS_INLINE explicit strSlice(const CHAR *str):
         _strLen(0),
         _str(str)
         {
            if (NULL != str)
            {
               _strLen = ossStrlen(str);
            }
         }

         OSS_INLINE ~strSlice()
         {
            _strLen = 0;
            _str = NULL;
         }

      public:
         OSS_INLINE strSlice &operator=(const strSlice &r)
         {
            _strLen = r._strLen;
            _str = r._str;
            return *this;
         }

         OSS_INLINE BOOLEAN operator==(const strSlice &r)const
         {
            return strLen() == r.strLen() &&
                   0 == ossStrcmp(str(), r.str());
         }

         OSS_INLINE UINT32 strLen()const
         {
            return empty() ? 0 : _strLen;
         }

         OSS_INLINE const CHAR *str()const
         {
            static const CHAR tmp = '\0';
            return empty() ? &tmp : _str;
         }

         OSS_INLINE void reset(const CHAR *str = NULL)
         {
            if (NULL != str)
            {
               _strLen = ossStrlen(str);
               _str = str;
            }
            else
            {
               _strLen = 0;
               _str = NULL;
            }
            return;
         }

         OSS_INLINE void reset(const CHAR *str, UINT32 size)
         {
            _str = str;
            _strLen = size;
            return;
         }

         OSS_INLINE BOOLEAN empty()const
         {
            return 0 == _strLen || NULL == _str;
         }
         
      private:
         UINT32 _strLen;
         const CHAR *_str;
   };//class strSlice

} // namespace vessel
} // namespace engine

#endif // VESSEL_STR_SLICE_H_
