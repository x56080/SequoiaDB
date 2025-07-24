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

   Source File Name = cursorOptions.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_CURSOR_OPTIONS_H_
#define VESSEL_CURSOR_OPTIONS_H_

#include "core.hpp"
#include "oss.hpp"
#include "ossTypes.hpp"

namespace engine
{
namespace vessel
{
   class cursorOptions : public SDBObject
   {
      public:
         cursorOptions(){}
          ~cursorOptions(){}
         cursorOptions(const cursorOptions &o):
         rowCountLimit(o.rowCountLimit),
         stepSize(o.stepSize),
         bufferSizeLimit(o.bufferSizeLimit)
         {}
         cursorOptions &operator=(const cursorOptions &o)
         {
            rowCountLimit = o.rowCountLimit;
            stepSize = o.stepSize;
            bufferSizeLimit = o.bufferSizeLimit;
            return *this;
         }

         OSS_INLINE BOOLEAN hasRowCountLimit()const
         {
            return 0 <= rowCountLimit;
         }
         OSS_INLINE BOOLEAN isValid()const
         {
            return 0 < stepSize &&
                   0 < bufferSizeLimit &&
                   0 < defaultBufferSize &&
                   defaultBufferSize <= bufferSizeLimit;
         }
      public:
         INT64 rowCountLimit = -1;
         UINT32 stepSize = 1024;
         UINT32 bufferSizeLimit = (UINT32)16 << 20; /// 16MB
         UINT32 defaultBufferSize = (UINT32)64 << 10; /// 64KB
   };
}//namespace vessel
}//namespace engine

#endif//VESSEL_CURSOR_OPTIONS_H_