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

   Source File Name = cursorOptions.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
         cursorOptions(const cursorOptions &) = delete;
         cursorOptions &operator=(const cursorOptions &o)
         {
            maxBufSize = o.maxBufSize;
            initBufSize = o.initBufSize;
            return *this;
         }

         ///cursor will try to extend buf only when the buf can not hold at
         /// least one slice.
         UINT32 maxBufSize = 16777216; /// 16MB
         UINT32 initBufSize = 32768;   /// 32KB
   };
}//namespace vessel
}//namespace engine

#endif//VESSEL_CURSOR_OPTIONS_H_